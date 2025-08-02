# dx - Enhance Developer Experience!
```
git init && git add . && git commit -m "feat: dx" && git branch -M main && git remote add origin https://github.com/manfromexistence/formatter-and-linter.git && git push -u origin main

find . -maxdepth 1 -mindepth 1 -exec du -sh {} + | sort -rh | sed 's/K/KB/; s/M/MB/; s|\./||'

find . -maxdepth 1 -mindepth 1 -exec du -sh {} + | sed 's/K/KB/; s/M/MB/; s|\./||'

find . -type d -name "tests" -exec rm -r {} +

find . -maxdepth 1 -mindepth 1 ! -name "cli" ! -name "src" ! -name "creates" ! -name "packages" -exec rm -rf {} +
```


```
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


int run_file_generator() {
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
```



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


### Rust with Rayon
```
// To run this code, you need to add `rayon` to your dependencies in Cargo.toml.
//
// [dependencies]
// rayon = "1.10.0"

use rayon::prelude::*;
use std::fs;
use std::path::Path;
use std::time::Instant;
use std::io::Result;

// --- Configuration ---
const NUM_FILES: u32 = 10_000;
const FOLDER_NAME: &str = "modules";
// -------------------

fn main() -> Result<()> {
    println!("Preparing to process {} files...", NUM_FILES);

    // Start the timer to measure the entire operation.
    let start_time = Instant::now();

    // Determine the message based on whether the directory already exists.
    let message_on_creation = if Path::new(FOLDER_NAME).exists() {
        "Files overwritten"
    } else {
        "Files created"
    };

    // Create the directory if it doesn't exist. `create_dir_all` is idempotent,
    // meaning it won't error if the directory is already there.
    fs::create_dir_all(FOLDER_NAME)?;
    println!("Directory '{}' is ready.", FOLDER_NAME);

    // This is where the magic happens. We create a range of numbers from 0 to NUM_FILES,
    // and `into_par_iter()` from the Rayon crate turns it into a parallel iterator.
    // Rayon's thread pool will automatically distribute the work of this loop
    // across multiple CPU cores.
    (0..NUM_FILES).into_par_iter().for_each(|i| {
        // Construct the full path for the new file.
        let file_path = format!("{}/file_{}.txt", FOLDER_NAME, i);

        // Define the content to be written to the file.
        let content = format!("{}\nThis is file number {}.", message_on_creation, i);

        // Write the content to the file. `fs::write` is a convenient function
        // that handles opening, writing, and closing the file.
        // We use `unwrap()` here for simplicity in the parallel loop. In a more
        // complex application, you might use `try_for_each` to handle errors.
        fs::write(&file_path, content)
            .unwrap_or_else(|e| eprintln!("Failed to write to {}: {}", file_path, e));
    });

    // Stop the timer.
    let duration = start_time.elapsed();

    // Report the results.
    println!("\n----------------------------------------");
    println!("Success!");
    println!("Action: {}", message_on_creation);
    println!("Files processed: {}", NUM_FILES);
    println!("Time taken: {} ms", duration.as_millis());
    println!("----------------------------------------");

    Ok(())
}
```

### Dx Styles
```
use notify::{Config, RecommendedWatcher, RecursiveMode, Watcher};
use std::fs::{self, OpenOptions};
use std::io::Write;
use std::path::Path;
use std::sync::mpsc::channel;
use std::time::{Duration, Instant};
use swc_common::{
    errors::{Handler},
    source_map::SourceMap,
    sync::Lrc,
    FileName,
    input::StringInput,
};
use swc_ecma_parser::{lexer::Lexer, Parser, Syntax, TsSyntax};
use swc_ecma_visit::{Visit, VisitWith};
use swc_ecma_ast;

unsafe extern "C" {
    fn run_file_generator(indices: *const i32, num_files: i32) -> i32;
}

// Visitor to extract className attributes from JSX
struct ClassNameVisitor {
    classes: Vec<String>,
}

impl ClassNameVisitor {
    fn new() -> Self {
        ClassNameVisitor { classes: Vec::new() }
    }
}

impl Visit for ClassNameVisitor {
    fn visit_jsx_opening_element(&mut self, n: &swc_ecma_ast::JSXOpeningElement) {
        for attr in &n.attrs {
            if let swc_ecma_ast::JSXAttrOrSpread::JSXAttr(attr) = attr {
                if let swc_ecma_ast::JSXAttrName::Ident(ident) = &attr.name {
                    if ident.sym.as_ref() == "className" {
                        if let Some(swc_ecma_ast::JSXAttrValue::Lit(lit)) = &attr.value {
                            if let swc_ecma_ast::Lit::Str(s) = lit {
                                // Split className string into individual classes
                                self.classes
                                    .extend(s.value.split_whitespace().map(|s| s.to_string()));
                            }
                        }
                    }
                }
            }
        }
    }
}

// Map Tailwind-like classes to CSS rules
fn class_to_css(class: &str) -> Option<String> {
    match class {
        "flex" => Some(format!(".{} {{ display: flex; }}", class)),
        // Add more Tailwind-like mappings here
        _ => None,
    }
}

// Update global.css with new CSS rules
fn update_global_css(classes: Vec<String>) -> std::io::Result<()> {
    let css_path = "styles/global.css";
    fs::create_dir_all("styles")?;

    // Collect unique CSS rules
    let mut css_rules = Vec::new();
    for class in classes {
        if let Some(rule) = class_to_css(&class) {
            if !css_rules.contains(&rule) {
                css_rules.push(rule);
            }
        }
    }

    // Write to global.css
    let mut file = OpenOptions::new()
        .write(true)
        .create(true)
        .truncate(true)
        .open(css_path)?;
    for rule in css_rules {
        writeln!(file, "{}", rule)?;
    }
    Ok(())
}

fn main() -> std::io::Result<()> {
    println!("🦀 Rust Observer: Initiating C file generator for 10000 files...");

    // Run the original file generator
    let indices: Vec<i32> = (0..10000).collect();
    let num_files = indices.len() as i32;
    let start_time = Instant::now();
    let status_code = unsafe { run_file_generator(indices.as_ptr(), num_files) };
    let duration = start_time.elapsed();

    if status_code == 0 {
        println!("🦀 Rust Observer: C generator finished successfully.");
        println!("🕒 Time taken for FFI operation: {:.2?}", duration);
        println!("\nMission Accomplished: Development experience enhanced!");
    } else {
        eprintln!(
            "🔥 Rust Observer: C generator reported an error (status code: {})!",
            status_code
        );
    }

    // Set up file watcher for .tsx files
    println!("🦀 Rust Observer: Starting file watcher for .tsx files...");
    let (tx, rx) = channel();
    let mut watcher = match RecommendedWatcher::new(
        tx,
        Config::default().with_poll_interval(Duration::from_millis(100)),
    ) {
        Ok(watcher) => watcher,
        Err(e) => {
            return Err(std::io::Error::new(
                std::io::ErrorKind::Other,
                format!("Failed to create watcher: {}", e),
            ));
        }
    };

    // Explicitly handle notify::Result without ?
    match watcher.watch(Path::new("test"), RecursiveMode::Recursive) {
        Ok(_) => (),
        Err(e) => {
            return Err(std::io::Error::new(
                std::io::ErrorKind::Other,
                format!("Watcher error: {}", e),
            ));
        }
    }

    // SWC setup
    let cm: Lrc<SourceMap> = Default::default();
    let handler = Handler::with_emitter_writer(
        Box::new(std::io::stderr()),
        Some(cm.clone()),
        // false,
        // ColorConfig::Auto,
    );

    // Watch loop
    for res in rx {
        match res {
            Ok(event) => {
                for path in event.paths {
                    if path.extension().and_then(|s| s.to_str()) == Some("tsx") {
                        println!("🦀 Detected change in: {:?}", path);

                        // Read and parse .tsx file
                        let content = match fs::read_to_string(&path) {
                            Ok(content) => content,
                            Err(e) => {
                                eprintln!("Error reading file {:?}: {}", path, e);
                                continue;
                            }
                        };

                        let fm = cm.new_source_file(
                            FileName::Custom(path.to_string_lossy().into()).into(),
                            content,
                        );
                        let lexer = Lexer::new(
                            Syntax::Typescript(TsSyntax {
                                tsx: true,
                                decorators: false,
                                dts: false,
                                no_early_errors: false,
                                disallow_ambiguous_jsx_like: false,
                            }),
                            Default::default(),
                            StringInput::from(&*fm),
                            None,
                        );
                        let mut parser = Parser::new_from(lexer);
                        let module = match parser.parse_module() {
                            Ok(module) => module,
                            Err(e) => {
                                e.into_diagnostic(&handler).emit();
                                continue;
                            }
                        };

                        // Extract class names
                        let mut visitor = ClassNameVisitor::new();
                        module.visit_with(&mut visitor);
                        if !visitor.classes.is_empty() {
                            println!("🦀 Found classes: {:?}", visitor.classes);
                            if let Err(e) = update_global_css(visitor.classes) {
                                eprintln!("Error updating global.css: {}", e);
                            } else {
                                println!("🦀 Updated styles/global.css");
                            }
                        }
                    }
                }
            }
            Err(e) => eprintln!("Watcher error: {:?}", e),
        }
    }

    Ok(())
}
```

[package]
name = "dx"
version = "0.1.0"
edition = "2024"

[dependencies]
notify = "8.1.0"
swc_common = "14.0.2"
swc_ecma_ast = "14.0.0"
swc_ecma_parser = "22.0.3"
swc_ecma_visit = "14.0.0"

[build-dependencies]
cc = "1.0"
