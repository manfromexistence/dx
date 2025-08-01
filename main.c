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

// Arguments for each worker thread.
typedef struct {
    int start_index;
    int end_index;
    int dir_fd; // File descriptor for the base directory
    const char *content;
    size_t content_len;
} ThreadArgs;

// A highly optimized integer-to-string function.
// It writes the number to the end of the buffer and returns a pointer to the start of the number.
static inline char* fast_itoa(int value, char* buffer_end) {
    *buffer_end = '\0';
    char* p = buffer_end;

    if (value == 0) {
        *--p = '0';
        return p;
    }
    
    // This loop is efficient for positive integers.
    do {
        *--p = '0' + (value % 10);
        value /= 10;
    } while (value > 0);

    return p;
}

// Worker for creating new files using write() and openat().
// Marked as 'hot' for the compiler to suggest aggressive optimization.
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

// Worker for overwriting existing files using mmap() and openat().
// Marked as 'hot' for the compiler to suggest aggressive optimization.
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

        // Open for read-write to allow memory mapping.
        int fd = openat(args->dir_fd, filename, O_RDWR);
        if (fd == -1) {
            continue;
        }

        // Truncate the file to the size of the new content.
        if (ftruncate(fd, args->content_len) == -1) {
            close(fd);
            continue;
        }

        // Map the file into memory.
        void *map = mmap(NULL, args->content_len, PROT_WRITE, MAP_SHARED, fd, 0);
        if (map == MAP_FAILED) {
            close(fd);
            continue;
        }

        // Copy the new content directly into the memory map.
        memcpy(map, args->content, args->content_len);

        // Unmap the file from memory, which flushes changes to disk.
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
    const char *content;
    const char *action_description;

    // Check if files exist to determine which worker function to use.
    if (faccessat(dir_fd, "file0.txt", F_OK, 0) == 0) {
        printf("INFO: Files exist. Using 'mmap' + 'openat' overwrite method.\n");
        worker_func = overwrite_files_mmap_worker;
        content = OVERWRITE_CONTENT;
        action_description = "overwriting";
    } else {
        printf("INFO: Files do not exist. Using 'write' + 'openat' creation method.\n");
        worker_func = create_files_worker;
        content = CREATE_CONTENT;
        action_description = "creating";
    }

    const size_t content_len = strlen(content);

    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];
    int files_per_thread = NUM_FILES / NUM_THREADS;

    printf("Starting file operations with %d threads...\n", NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].start_index = i * files_per_thread;
        args[i].end_index = (i == NUM_THREADS - 1) ? NUM_FILES : (i + 1) * files_per_thread;
        args[i].dir_fd = dir_fd;
        args[i].content = content;
        args[i].content_len = content_len;
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
