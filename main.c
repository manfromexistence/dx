#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <libaio.h>
#include <pthread.h>

#define NUM_FILES 1000
#define NUM_THREADS 4
#define FOLDER "modules/"
#define CONTENT "hello world\n"
#define MAX_AIO_EVENTS 128

typedef struct {
    int start;
    int end;
} ThreadArgs;

void *create_files(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    char filename[256];
    const char *content = CONTENT;
    size_t content_len = strlen(content);

    io_context_t ctx = 0;
    if (io_setup(MAX_AIO_EVENTS, &ctx) < 0) {
        perror("io_setup failed");
        return NULL;
    }

    struct iocb *iocbs[MAX_AIO_EVENTS];
    struct io_event events[MAX_AIO_EVENTS];
    struct iocb cb[MAX_AIO_EVENTS];
    int fd[MAX_AIO_EVENTS];
    int num_events = 0;

    for (int i = args->start; i < args->end; i++) {
        snprintf(filename, sizeof(filename), "%sfile%d.txt", FOLDER, i);
        fd[num_events] = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd[num_events] == -1) {
            perror("Error opening file");
            continue;
        }

        iocbs[num_events] = &cb[num_events];
        io_prep_pwrite(&cb[num_events], fd[num_events], (void *)content, content_len, 0);
        num_events++;

        if (num_events == MAX_AIO_EVENTS || i == args->end - 1) {
            if (io_submit(ctx, num_events, iocbs) != num_events) {
                perror("io_submit failed");
                for (int j = 0; j < num_events; j++) {
                    close(fd[j]);
                }
                io_destroy(ctx);
                return NULL;
            }

            int ret;
            while ((ret = io_getevents(ctx, 1, num_events, events, NULL)) > 0) {
                for (int j = 0; j < ret; j++) {
                    fdatasync(fd[j]);
                    close(fd[j]);
                }
            }
            num_events = 0;
        }
    }

    io_destroy(ctx);
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