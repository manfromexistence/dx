#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

#define NUM_FILES 1000
#define NUM_THREADS 4
#define FOLDER "modules"
#define CONTENT "hello world\n"

typedef struct {
    int start;
    int end;
} ThreadArgs;

void *create_files(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    char filename[256] = FOLDER "/file";
    char num_buf[16];
    const char *content = CONTENT;
    size_t content_len = strlen(content);
    int folder_len = strlen(FOLDER);

    for (int i = args->start; i < args->end; i++) {
        snprintf(num_buf, sizeof(num_buf), "%d.txt", i);
        strcpy(filename + folder_len + 5, num_buf);

        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("Error opening file");
            continue;
        }

        if (write(fd, content, content_len) == -1) {
            perror("Error writing to file");
            close(fd);
            continue;
        }

        close(fd);
    }
    return NULL;
}

int main() {
    // Measure start time
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Check if modules directory exists
    struct stat st;
    if (stat(FOLDER, &st) == -1) {
        if (errno == ENOENT) {
            if (mkdir(FOLDER, 0755) == -1) {
                perror("Error creating directory");
                return 1;
            }
        } else {
            perror("Error checking directory");
            return 1;
        }
    }

    // Create threads
    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];
    int files_per_thread = NUM_FILES / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].start = i * files_per_thread;
        args[i].end = (i + 1) * files_per_thread;
        if (i == NUM_THREADS - 1) {
            args[i].end = NUM_FILES; // Handle remainder
        }
        pthread_create(&threads[i], NULL, create_files, &args[i]);
    }

    // Wait for threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Measure end time
    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calculate time in milliseconds
    double time_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                     (end.tv_nsec - start.tv_nsec) / 1000000.0;
    printf("Time taken: %.2f ms\n", time_ms);

    return 0;
}