#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>

#define NUM_FILES 1000
#define NUM_THREADS 4
#define FOLDER "modules/"
#define CONTENT "hello world\n"
#define BATCH_SIZE 64

typedef struct {
    int start;
    int end;
} ThreadArgs;

void *create_files(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    char filename[256];
    const char *content = CONTENT;
    size_t content_len = strlen(content);
    int fd[BATCH_SIZE];
    int num_files = 0;

    for (int i = args->start; i < args->end; i++) {
        snprintf(filename, sizeof(filename), "%sfile%d.txt", FOLDER, i);
        fd[num_files] = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd[num_files] == -1) {
            fprintf(stderr, "Error opening file %s: %s\n", filename, strerror(errno));
            continue;
        }
        num_files++;

        if (num_files == BATCH_SIZE || i == args->end - 1) {
            printf("Thread %ld writing %d files at file %d\n", pthread_self(), num_files, i);
            for (int j = 0; j < num_files; j++) {
                if (write(fd[j], content, content_len) == -1) {
                    fprintf(stderr, "Error writing to file %d: %s\n", i - num_files + j + 1, strerror(errno));
                }
                close(fd[j]);
            }
            num_files = 0;
        }
    }

    return NULL;
}

int main() {
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    struct stat st;
    if (stat(FOLDER, &st) == -1) {
        if (errno == ENOENT) {
            if (mkdir(FOLDER, 0755) == -1) {
                perror("Error creating directory");
                return 1;
            }

            char temp_file[256];
            snprintf(temp_file, sizeof(temp_file), "%stemp.dat", FOLDER);
            int fd = open(temp_file, O_WRONLY | O_CREAT, 0644);
            if (fd != -1) {
                if (fallocate(fd, 0, 0, 1024 * 1024) == -1) {
                    perror("fallocate failed");
                }
                close(fd);
                unlink(temp_file);
            }
        } else {
            perror("Error checking directory");
            return 1;
        }
    }

    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];
    int files_per_thread = NUM_FILES / NUM_THREADS;

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].start = i * files_per_thread;
        args[i].end = (i + 1) * files_per_thread;
        if (i == NUM_THREADS - 1) {
            args[i].end = NUM_FILES;
        }
        pthread_create(&threads[i], NULL, create_files, &args[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                     (end.tv_nsec - start.tv_nsec) / 1000000.0;
    printf("Time taken: %.2f ms\n", time_ms);

    return 0;
}