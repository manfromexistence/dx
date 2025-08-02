# dx - Enhance Developer Experience!
```
git init && git add . && git commit -m "feat: dx" && git branch -M main && git remote add origin https://github.com/manfromexistence/formatter-and-linter.git && git push -u origin main

find . -maxdepth 1 -mindepth 1 -exec du -sh {} + | sort -rh | sed 's/K/KB/; s/M/MB/; s|\./||'

find . -maxdepth 1 -mindepth 1 -exec du -sh {} + | sed 's/K/KB/; s/M/MB/; s|\./||'

find . -type d -name "tests" -exec rm -r {} +

find . -maxdepth 1 -mindepth 1 ! -name "cli" ! -name "src" ! -name "creates" ! -name "packages" -exec rm -rf {} +
```


Remove all comments from this rust and don't change anything for now!

```
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <threads.h>
#include <errno.h>

#ifdef _WIN32
#include <direct.h>
#define MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>
#define MKDIR(path) mkdir(path, 0777)
#endif

#define NUM_FILES 10000
#define NUM_THREADS 8
#define DIR_NAME "modules"
#define FILE_CONTENT "Hello, World!"

typedef struct
{
    int start_index;
    int end_index;
} ThreadData;

double get_monotonic_time()
{
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1.0e9;
}

int create_files_worker(void *arg)
{
    ThreadData *data = (ThreadData *)arg;
    char filepath[256];
    const char *content = FILE_CONTENT;

    for (int i = data->start_index; i < data->end_index; ++i)
    {
        snprintf(filepath, sizeof(filepath), "%s/file_%d.txt", DIR_NAME, i);

        FILE *fp = fopen(filepath, "w");
        if (fp == NULL)
        {
            fprintf(stderr, "Error: Could not open file %s\n", filepath);
            continue;
        }

        fputs(content, fp);
        fclose(fp);
    }
    return 0;
}

int main()
{
    printf("Starting portable file creation challenge using C11 threads...\n");

    double start_time = get_monotonic_time();

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

    printf("Using a fixed number of %d threads.\n", NUM_THREADS);
    thrd_t threads[NUM_THREADS];
    ThreadData thread_data_array[NUM_THREADS];

    int files_per_thread = NUM_FILES / NUM_THREADS;
    int remainder_files = NUM_FILES % NUM_THREADS;

    int current_start_index = 0;
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        thread_data_array[i].start_index = current_start_index;
        int files_for_this_thread = files_per_thread + (i < remainder_files ? 1 : 0);
        thread_data_array[i].end_index = current_start_index + files_for_this_thread;
        current_start_index = thread_data_array[i].end_index;

        if (thrd_create(&threads[i], create_files_worker, &thread_data_array[i]) != thrd_success)
        {
            fprintf(stderr, "Error: Failed to create thread %d.\n", i);
        }
    }

    printf("All threads launched. Waiting for them to finish...\n");
    for (int i = 0; i < NUM_THREADS; ++i)
    {
        thrd_join(threads[i], NULL);
    }

    double end_time = get_monotonic_time();
    double elapsed_time = end_time - start_time;

    printf("\n----------------------------------------\n");
    printf("          MISSION COMPLETE\n");
    printf("----------------------------------------\n");
    printf("Created %d files in the '%s' directory.\n", NUM_FILES, DIR_NAME);
    printf("Total time taken: %.0f ms\n", elapsed_time * 1000);
    printf("----------------------------------------\n");

    return 0;
}

```