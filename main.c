#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#define NUM_FILES 1000
#define NUM_THREADS 8
#define FOLDER "modules/"
#define CREATE_CONTENT "Files Created!\n"
#define OVERWRITE_CONTENT "Files Overwritten!\n"

typedef struct {
    int start_index;
    int end_index;
} ThreadArgs;

void *create_files_worker(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    char filepath[512];
    const char *content = CREATE_CONTENT;
    size_t content_len = strlen(CREATE_CONTENT);

    for (int i = args->start_index; i < args->end_index; i++) {
        snprintf(filepath, sizeof(filepath), "%sfile%d.txt", FOLDER, i);
        int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (fd == -1) {
            fprintf(stderr, "Thread %ld: Error opening file %s: %s\n",
                    pthread_self(), filepath, strerror(errno));
            continue;
        }

        if (write(fd, content, content_len) == -1) {
            fprintf(stderr, "Thread %ld: Error writing to file %s: %s\n",
                    pthread_self(), filepath, strerror(errno));
        }
        close(fd);
    }
    return NULL;
}

void *overwrite_files_mmap_worker(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    char filepath[512];
    const char *content = OVERWRITE_CONTENT;
    size_t content_len = strlen(OVERWRITE_CONTENT);

    for (int i = args->start_index; i < args->end_index; i++) {
        snprintf(filepath, sizeof(filepath), "%sfile%d.txt", FOLDER, i);

        int fd = open(filepath, O_RDWR | O_CREAT, 0644);
        if (fd == -1) {
            fprintf(stderr, "Thread %ld: (mmap) Error opening file %s: %s\n",
                    pthread_self(), filepath, strerror(errno));
            continue;
        }

        if (ftruncate(fd, content_len) == -1) {
            fprintf(stderr, "Thread %ld: (mmap) Error truncating file %s: %s\n",
                    pthread_self(), filepath, strerror(errno));
            close(fd);
            continue;
        }

        void *map = mmap(NULL, content_len, PROT_WRITE, MAP_SHARED, fd, 0);
        if (map == MAP_FAILED) {
            fprintf(stderr, "Thread %ld: (mmap) Error mapping file %s: %s\n",
                    pthread_self(), filepath, strerror(errno));
            close(fd);
            continue;
        }

        memcpy(map, content, content_len);

        munmap(map, content_len);

        close(fd);
    }
    return NULL;
}

int main() {
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    struct stat st;
    int folder_exists = 0;
    if (stat(FOLDER, &st) == -1) {
        if (errno == ENOENT) {
            printf("INFO: Folder does not exist. Creating '%s'...\n", FOLDER);
            if (mkdir(FOLDER, 0755) == -1) {
                perror("Fatal: Could not create directory");
                return 1;
            }
        } else {
            perror("Fatal: Could not stat directory");
            return 1;
        }
    } else {
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "Fatal: '%s' exists but is not a directory.\n", FOLDER);
            return 1;
        }
        folder_exists = 1;
    }

    void *(*worker_func)(void *);
    if (folder_exists) {
        printf("INFO: Folder exists. Using fast 'mmap' overwrite method.\n");
        worker_func = overwrite_files_mmap_worker;
    } else {
        printf("INFO: Using standard 'write' method for initial creation.\n");
        worker_func = create_files_worker;
    }

    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];
    int files_per_thread = NUM_FILES / NUM_THREADS;

    printf("Starting file creation with %d threads...\n", NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].start_index = i * files_per_thread;
        args[i].end_index = (i == NUM_THREADS - 1) ? NUM_FILES : (i + 1) * files_per_thread;
        pthread_create(&threads[i], NULL, worker_func, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double time_ms = (end_time.tv_sec - start_time.tv_sec) * 1000.0 +
                     (end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;

    printf("\nFinished creating/updating %d files.\n", NUM_FILES);
    printf("Total time taken: %.2f ms\n", time_ms);

    return 0;
}
