// src/main.rs

// Import the Instant struct from the standard library to measure time.
use std::time::Instant;

// This `extern "C"` block is the Foreign Function Interface (FFI) boundary.
// It declares the signature of the C function we want to call.
unsafe extern "C" {
    // This function is defined in our `src/generator/main.c` file.
    fn run_file_generator() -> i32;
}

fn main() {
    println!("🦀 Rust Observer: Initiating C file generator...");

    // Start the timer right before calling the C function.
    let start_time = Instant::now();

    // Calling a C function is an `unsafe` operation in Rust.
    // We are telling the compiler that we guarantee this call is safe.
    let status_code = unsafe {
        run_file_generator()
    };
    
    // Stop the timer and calculate the elapsed duration.
    let duration = start_time.elapsed();

    // Check the status code returned by the C function to see if it succeeded.
    if status_code == 0 {
        println!("🦀 Rust Observer: C generator finished successfully.");
        // Print the time taken for the FFI call.
        println!("🕒 Time taken for FFI operation: {:.2?}", duration);
        println!("\nMission Accomplished: Development experience enhanced!");
    } else {
        eprintln!("🔥 Rust Observer: C generator reported an error (status code: {})!", status_code);
    }
}
