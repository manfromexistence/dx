// fn main(){
//     println!("Hello, essensefromexistence")
// }

use rayon::prelude::*;      // Import the parallel iterator traits from Rayon.
use std::fs::{create_dir_all, OpenOptions}; // For creating directories and files.
use std::path::Path;        // For handling file system paths.
use std::time::Instant;     // To benchmark the execution time.

// The content we want to write into each file.
const FILE_CONTENT: &[u8] = b"Hello, essencefromexsitence";
// The number of files we want to create.
const NUM_FILES: usize = 1000;
// The directory where the files will be stored.
const MODULES_DIR: &str = "modules";

fn main() -> std::io::Result<()> {
    println!("Preparing to create {} files...", NUM_FILES);

    // --- 1. Setup the Directory ---
    // First, we create the 'modules' directory.
    // `create_dir_all` is convenient because it doesn't return an error if the directory already exists.
    let dir_path = Path::new(MODULES_DIR);
    if !dir_path.exists() {
        println!("Creating directory: '{}'", MODULES_DIR);
        create_dir_all(dir_path)?;
    }

    // Start the timer to see how fast this operation is.
    let start_time = Instant::now();

    // --- 2. Parallel File Creation ---
    // We use Rayon's `par_iter` to create a parallel iterator.
    // This will distribute the work of creating 1000 files across all available CPU cores.
    (0..NUM_FILES).into_par_iter().for_each(|i| {
        // Construct the full path for each file (e.g., "modules/file_0.txt")
        let file_path = dir_path.join(format!("file_{}.txt", i));

        // The core logic for creating and writing a single file.
        // We wrap this in a closure to handle potential errors for each file individually.
        let create_and_write = || -> std::io::Result<()> {
            // Open the file with options to read, write, create, and truncate.
            // `create(true)` will create the file if it doesn't exist.
            // `truncate(true)` will clear the file if it already exists.
            let file = OpenOptions::new()
                .read(true)
                .write(true)
                .create(true)
                .truncate(true)
                .open(&file_path)?;

            // Set the file's length to the size of our content.
            // This is a crucial step for memory mapping, as the file needs to have a defined size on disk
            // before we can map it into memory.
            file.set_len(FILE_CONTENT.len() as u64)?;

            // Create a mutable memory map of the file.
            // This maps the file on disk directly to a slice in memory (`&mut [u8]`).
            // Writing to this slice is extremely fast as it's a direct memory operation.
            let mut mmap = unsafe { memmap2::MmapMut::map_mut(&file)? };

            // Copy our content into the memory-mapped slice.
            // The operating system will handle flushing this memory back to the disk efficiently.
            mmap[..FILE_CONTENT.len()].copy_from_slice(FILE_CONTENT);
            
            // Explicitly flush the changes to disk to ensure they are written.
            // While the OS handles this automatically when the map is dropped,
            // an explicit flush can be useful for guaranteed persistence.
            mmap.flush()?;

            Ok(())
        };

        // Execute the file creation logic and handle any errors.
        if let Err(e) = create_and_write() {
            // In a real-world application, you might want more sophisticated error handling,
            // like collecting failed file attempts. For this speed demo, we'll just print the error.
            eprintln!("Failed to create file {:?}: {}", file_path, e);
        }
    });

    // Stop the timer and calculate the duration.
    let duration = start_time.elapsed();

    println!("\n---------------------------------");
    println!("      Task Complete!");
    println!("---------------------------------");
    println!(
        "Successfully created {} files in {:.2?}.",
        NUM_FILES, duration
    );
    println!("You can find them in the '{}' directory.", MODULES_DIR);

    Ok(())
}
