/**
 * @file fast_file_generator.c
 * @brief Creates 10,000 files as quickly as possible using multithreading and low-level I/O.
 *
 * This program demonstrates high-speed, concurrent file I/O. It is designed
 * to be cross-platform, working on both POSIX systems (Linux, macOS) and Windows.
 *
 * How it works:
 * 1. Uses a fixed number of 8 threads for file creation.
 * 2. Creates a directory named "modules".
 * 3. Divides the task of creating 10,000 files among the 8 threads.
 * 4. Each thread uses low-level, unbuffered system calls for maximum speed:
 * - On POSIX: open(), write()
 * - On Windows: CreateFileA(), WriteFile()
 * This reduces the overhead compared to standard library functions like fopen().
 * 5. Measures the total time taken for the entire operation using a high-resolution
 * monotonic clock.
 *
 * Compilation:
 * - On Linux/macOS: gcc fast_file_generator.c -o generator -pthread
 * - On Windows (with MinGW): gcc fast_file_generator.c -o generator.exe
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> // For checking error numbers like EEXIST

// Platform-specific includes and definitions
#ifdef _WIN32
#include <windows.h>
#include <direct.h> // For _mkdir
#define MKDIR(path) _mkdir(path)
#else
#include <pthread.h>
#include <sys/stat.h> // For mkdir
#include <sys/types.h>
#include <unistd.h> // For sysconf, write, close
#include <fcntl.h>  // For open
#define MKDIR(path) mkdir(path, 0777)
#endif

// --- Configuration ---
#define NUM_FILES 10000
#define NUM_THREADS 8 // Hardcoded to 8 threads as requested
#define DIR_NAME "modules"
#define FILE_CONTENT "Hello, World!"

/**
 * @struct ThreadData
 * @brief  A structure to pass information to each worker thread.
 *
 * This struct tells each thread which range of files it is responsible for creating.
 */
typedef struct
{
    int start_index;
    int end_index;
} ThreadData;

/**
 * @brief Gets the current time from a high-resolution monotonic clock.
 *
 * A monotonic clock is not affected by system time changes (e.g., daylight saving)
 * and is ideal for measuring time intervals.
 *
 * @return The current time in seconds as a double.
 */
double get_monotonic_time()
{
#ifdef _WIN32
    // Windows-specific high-resolution timer
    static LARGE_INTEGER frequency;
    // Get the frequency only once
    if (frequency.QuadPart == 0)
    {
        QueryPerformanceFrequency(&frequency);
    }
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)frequency.QuadPart;
#else
    // POSIX-specific high-resolution timer
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1.0e9;
#endif
}

/**
 * @brief The worker function executed by each thread. (OPTIMIZED)
 *
 * This function uses low-level system calls (open/write on POSIX, CreateFile/WriteFile on Windows)
 * to create files, which is generally faster for creating many small files than using
 * the standard C library's buffered I/O (fopen/fputs).
 *
 * @param arg A void pointer to a ThreadData struct.
 * @return A status code (0 for success).
 */
#ifdef _WIN32
DWORD WINAPI create_files_worker(LPVOID arg)
#else
void *create_files_worker(void *arg)
#endif
{
    ThreadData *data = (ThreadData *)arg;
    char filepath[256];
    const char *content = FILE_CONTENT;
    size_t content_len = strlen(content);

    // printf("Thread starting: files %d to %d\n", data->start_index, data->end_index - 1);

    for (int i = data->start_index; i < data->end_index; ++i)
    {
        // Safely format the file path string
        snprintf(filepath, sizeof(filepath), "%s/file_%d.txt", DIR_NAME, i);

#ifdef _WIN32
        // --- Windows Low-Level File I/O ---
        HANDLE hFile = CreateFileA(filepath,           // file to create
                                   GENERIC_WRITE,      // open for writing
                                   0,                  // do not share
                                   NULL,               // default security
                                   CREATE_ALWAYS,      // overwrite existing
                                   FILE_ATTRIBUTE_NORMAL, // normal file
                                   NULL);              // no attr. template

        if (hFile == INVALID_HANDLE_VALUE)
        {
            fprintf(stderr, "Error: Could not create file %s (Win32 Error: %lu)\n", filepath, GetLastError());
            continue;
        }

        DWORD bytesWritten;
        WriteFile(hFile, content, (DWORD)content_len, &bytesWritten, NULL);
        CloseHandle(hFile);
#else
        // --- POSIX Low-Level File I/O ---
        int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd == -1)
        {
            // If file creation fails, print an error and continue.
            fprintf(stderr, "Error: Could not open file %s\n", filepath);
            continue;
        }

        write(fd, content, content_len);
        close(fd);
#endif
    }

#ifdef _WIN32
    return 0;
#else
    return NULL;
#endif
}

/**
 * @brief Main entry point of the program.
 */
int main()
{
    printf("Starting optimized file creation challenge...\n");

    // --- 1. Start Timer ---
    double start_time = get_monotonic_time();

    // --- 2. Create Directory ---
    if (MKDIR(DIR_NAME) != 0)
    {
        if (errno != EEXIST)
        {
            perror("Error: Failed to create directory");
            return 1;
        }
        printf("Directory '%s' already exists. Continuing...\n", DIR_NAME);
    }
    else
    {
        printf("Directory '%s' created successfully.\n", DIR_NAME);
    }

    // --- 3. Setup Threads ---
    printf("Using a fixed number of %d threads.\n", NUM_THREADS);
#ifdef _WIN32
    HANDLE *threads = malloc(NUM_THREADS * sizeof(HANDLE));
#else
    pthread_t *threads = malloc(NUM_THREADS * sizeof(pthread_t));
#endif
    ThreadData *thread_data_array = malloc(NUM_THREADS * sizeof(ThreadData));

    if (threads == NULL || thread_data_array == NULL)
    {
        fprintf(stderr, "Error: Failed to allocate memory for threads.\n");
        free(threads);
        free(thread_data_array);
        return 1;
    }

    int files_per_thread = NUM_FILES / NUM_THREADS;
    int remainder_files = NUM_FILES % NUM_THREADS;

    // --- 4. Launch Threads ---
    int current_start_index = 0;
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        thread_data_array[i].start_index = current_start_index;
        int files_for_this_thread = files_per_thread + (i < remainder_files ? 1 : 0);
        thread_data_array[i].end_index = current_start_index + files_for_this_thread;
        current_start_index = thread_data_array[i].end_index;

#ifdef _WIN32
        threads[i] = CreateThread(NULL, 0, create_files_worker, &thread_data_array[i], 0, NULL);
        if (threads[i] == NULL)
        {
            fprintf(stderr, "Error: Failed to create thread %d.\n", i);
        }
#else
        int result = pthread_create(&threads[i], NULL, create_files_worker, &thread_data_array[i]);
        if (result != 0)
        {
            fprintf(stderr, "Error: Failed to create thread %d. Code: %d\n", i, result);
        }
#endif
    }

    // --- 5. Wait for Threads to Complete ---
    printf("All threads launched. Waiting for them to finish...\n");
#ifdef _WIN32
    WaitForMultipleObjects(NUM_THREADS, threads, TRUE, INFINITE);
    for (int i = 0; i < NUM_THREADS; i++)
    {
        CloseHandle(threads[i]);
    }
#else
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        pthread_join(threads[i], NULL);
    }
#endif

    // --- 6. Stop Timer and Report ---
    double end_time = get_monotonic_time();
    double elapsed_time = end_time - start_time;

    printf("\n----------------------------------------\n");
    printf("          MISSION COMPLETE\n");
    printf("----------------------------------------\n");
    printf("Created %d files in the '%s' directory.\n", NUM_FILES, DIR_NAME);
    printf("Total time taken: %.4f seconds\n", elapsed_time);
    printf("----------------------------------------\n");

    // --- 7. Cleanup ---
    free(threads);
    free(thread_data_array);

    return 0;
}
