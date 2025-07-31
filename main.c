#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#define NUM_FILES 1000
#define FOLDER "modules/"
#define CONTENT "hello world\n"

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

    char filename[256];
    const char *content = CONTENT;
    size_t content_len = strlen(content);

    for (int i = 0; i < NUM_FILES; i++) {
        // Construct filename (e.g., modules/file0.txt)
        snprintf(filename, sizeof(filename), "%sfile%d.txt", FOLDER, i);

        // Open file with low-level syscall
        int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd == -1) {
            perror("Error opening file");
            continue;
        }

        // Write content
        if (write(fd, content, content_len) == -1) {
            perror("Error writing to file");
            close(fd);
            continue;
        }

        // Ensure data is written (faster than fsync)
        fdatasync(fd);

        // Close file
        close(fd);
    }

    // Measure end time
    clock_gettime(CLOCK_MONOTONIC, &end);

    // Calculate time in milliseconds
    double time_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                     (end.tv_nsec - start.tv_nsec) / 1000000.0;
    printf("Time taken: %.2f ms\n", time_ms);

    return 0;
}