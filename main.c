#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>

#define NUM_FILES 10000
#define NUM_THREADS 8
#define FOLDER "modules/"
#define FILE_PREFIX "file"
#define FILE_SUFFIX ".txt"
#define CREATE_CONTENT "Files Created!\n"
#define OVERWRITE_CONTENT "Files Overwritten!\n"

typedef struct {
    int start_index;
    int end_index;
    int dir_fd;
    const char *content;
    size_t content_len;
} ThreadArgs;

static inline char* fast_itoa(int value, char* buffer_end) {
    *buffer_end = '\0';
    char* p = buffer_end;

    if (value == 0) {
        *--p = '0';
        return p;
    }

    do {
        *--p = '0' + (value % 10);
        value /= 10;
    } while (value > 0);

    return p;
}

void *create_files_worker(void *arg) __attribute__((hot));
void *create_files_worker(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    
    char filename[256];
    const size_t prefix_len = strlen(FILE_PREFIX);
    const size_t suffix_len = strlen(FILE_SUFFIX);
    
    memcpy(filename, FILE_PREFIX, prefix_len);
    char *num_start_ptr = filename + prefix_len;

    for (int i = args->start_index; i < args->end_index; i++) {
        char num_buf[12];
        char* num_str = fast_itoa(i, num_buf + sizeof(num_buf) - 1);
        size_t num_len = (num_buf + sizeof(num_buf) - 1) - num_str;

        memcpy(num_start_ptr, num_str, num_len);
        memcpy(num_start_ptr + num_len, FILE_SUFFIX, suffix_len + 1);

        int fd = openat(args->dir_fd, filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            continue;
        }

        write(fd, args->content, args->content_len);
        close(fd);
    }
    return NULL;
}

void *overwrite_files_mmap_worker(void *arg) __attribute__((hot));
void *overwrite_files_mmap_worker(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    
    char filename[256];
    const size_t prefix_len = strlen(FILE_PREFIX);
    const size_t suffix_len = strlen(FILE_SUFFIX);

    memcpy(filename, FILE_PREFIX, prefix_len);
    char *num_start_ptr = filename + prefix_len;

    for (int i = args->start_index; i < args->end_index; i++) {
        char num_buf[12];
        char* num_str = fast_itoa(i, num_buf + sizeof(num_buf) - 1);
        size_t num_len = (num_buf + sizeof(num_buf) - 1) - num_str;

        memcpy(num_start_ptr, num_str, num_len);
        memcpy(num_start_ptr + num_len, FILE_SUFFIX, suffix_len + 1);

        int fd = openat(args->dir_fd, filename, O_RDWR);
        if (fd == -1) {
            continue;
        }

        void *map = mmap(NULL, args->content_len, PROT_WRITE, MAP_SHARED, fd, 0);
        if (map == MAP_FAILED) {
            close(fd);
            continue;
        }

        memcpy(map, args->content, args->content_len);
        munmap(map, args->content_len);
        close(fd);
    }
    return NULL;
}


int main() {
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    mkdir(FOLDER, 0755);

    int dir_fd = open(FOLDER, O_RDONLY | O_DIRECTORY);
    if (dir_fd == -1) {
        perror("Fatal: Could not open directory " FOLDER);
        return 1;
    }

    void *(*worker_func)(void *);
    const char *content_to_write;
    const char *action_description;

    const size_t create_len = strlen(CREATE_CONTENT);
    const size_t overwrite_len = strlen(OVERWRITE_CONTENT);
    const size_t max_len = (create_len > overwrite_len) ? create_len : overwrite_len;

    char padded_create_content[max_len + 1];
    char padded_overwrite_content[max_len + 1];

    memcpy(padded_create_content, CREATE_CONTENT, create_len);
    memset(padded_create_content + create_len, ' ', max_len - create_len);
    padded_create_content[max_len] = '\0';

    memcpy(padded_overwrite_content, OVERWRITE_CONTENT, overwrite_len);
    memset(padded_overwrite_content + overwrite_len, ' ', max_len - overwrite_len);
    padded_overwrite_content[max_len] = '\0';

    if (faccessat(dir_fd, "file0.txt", F_OK, 0) == 0) {
        printf("INFO: Files exist. Using 'mmap' + 'openat' overwrite method.\n");
        worker_func = overwrite_files_mmap_worker;
        content_to_write = padded_overwrite_content;
        action_description = "overwriting";
    } else {
        printf("INFO: Files do not exist. Using 'write' + 'openat' creation method.\n");
        worker_func = create_files_worker;
        content_to_write = padded_create_content;
        action_description = "creating";
    }

    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];
    int files_per_thread = NUM_FILES / NUM_THREADS;

    printf("Starting file operations with %d threads...\n", NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].start_index = i * files_per_thread;
        args[i].end_index = (i == NUM_THREADS - 1) ? NUM_FILES : (i + 1) * files_per_thread;
        args[i].dir_fd = dir_fd;
        args[i].content = content_to_write;
        args[i].content_len = max_len;
        pthread_create(&threads[i], NULL, worker_func, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    close(dir_fd);

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double time_ms = (end_time.tv_sec - start_time.tv_sec) * 1000.0 + 
                     (end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;

    printf("\nFinished %s %d files.\n", action_description, NUM_FILES);
    printf("Total time taken: %.2f ms\n", time_ms);

    return 0;
}
