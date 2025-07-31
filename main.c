#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h> // Required for mode_t on some systems
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#define NUM_FILES 1000
#define NUM_THREADS 8
#define FOLDER "modules/"
#define CONTENT "hello world from portable C code\n"

// --- Common Structures ---

// Arguments for each thread
typedef struct {
    int start_index;
    int end_index;
} ThreadArgs;


// --- Portable File Creation Method ---
// This worker function is designed for maximum portability. It uses
// standard open, write, and close calls that are available on any
// POSIX-compliant system (like Linux, macOS, BSD) and have
// equivalents on Windows.

void *create_files_worker(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    char filepath[512]; // Increased buffer size for safety
    const char *content = CONTENT;
    size_t content_len = strlen(content);

    for (int i = args->start_index; i < args->end_index; i++) {
        // Construct the full path for the file to be created.
        snprintf(filepath, sizeof(filepath), "%sfile%d.txt", FOLDER, i);

        // Open the file with flags to create it if it doesn't exist,
        // truncate it if it does, and make it write-only.
        // The mode 0644 sets the file permissions (read/write for owner, read for others).
        int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);

        if (fd == -1) {
            // If opening the file fails, print an error and move to the next file.
            fprintf(stderr, "Thread %ld: Error opening file %s: %s\n",
                    pthread_self(), filepath, strerror(errno));
            continue;
        }

        // Write the predefined content to the file.
        if (write(fd, content, content_len) == -1) {
            fprintf(stderr, "Thread %ld: Error writing to file %s: %s\n",
                    pthread_self(), filepath, strerror(errno));
        }

        // Always close the file descriptor when done.
        close(fd);
    }
    return NULL;
}


// --- Main Application Logic ---

int main() {
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // --- Directory Setup ---
    // Check if the target directory exists.
    struct stat st;
    if (stat(FOLDER, &st) == -1) {
        // If it doesn't exist (ENOENT), create it.
        if (errno == ENOENT) {
            printf("INFO: Folder does not exist. Creating '%s'...\n", FOLDER);
            if (mkdir(FOLDER, 0755) == -1) {
                perror("Fatal: Could not create directory");
                return 1;
            }
        } else {
            // For any other stat error, exit.
            perror("Fatal: Could not stat directory");
            return 1;
        }
    } else {
        // If stat succeeds, check if it's actually a directory.
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "Fatal: '%s' exists but is not a directory.\n", FOLDER);
            return 1;
        }
        printf("INFO: Folder '%s' already exists. Proceeding to create files.\n", FOLDER);
    }


    // --- Threaded File Creation ---
    pthread_t threads[NUM_THREADS];
    ThreadArgs args[NUM_THREADS];
    int files_per_thread = NUM_FILES / NUM_THREADS;

    printf("Starting file creation with %d threads...\n", NUM_THREADS);

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].start_index = i * files_per_thread;
        // Ensure the last thread handles any remaining files.
        args[i].end_index = (i == NUM_THREADS - 1) ? NUM_FILES : (i + 1) * files_per_thread;

        pthread_create(&threads[i], NULL, create_files_worker, &args[i]);
    }

    // Wait for all threads to complete their work.
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // --- Timing and Completion ---
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double time_ms = (end_time.tv_sec - start_time.tv_sec) * 1000.0 +
                     (end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;

    printf("\nFinished creating %d files.\n", NUM_FILES);
    printf("Total time taken: %.2f ms\n", time_ms);

    return 0;
}
