use std::time::Instant;

unsafe extern "C" {
    fn run_file_generator(indices: *const i32, num_files: i32) -> i32;
}

fn main() {
    println!("🦀 Rust Observer: Initiating C file generator for 10000 files...");

    let indices: Vec<i32> = (0..10000).collect();
    let num_files = indices.len() as i32;

    let start_time = Instant::now();

    let status_code = unsafe {
        run_file_generator(indices.as_ptr(), num_files)
    };

    let duration = start_time.elapsed();

    if status_code == 0 {
        println!("🦀 Rust Observer: C generator finished successfully.");
        println!("🕒 Time taken for FFI operation: {:.2?}", duration);
        println!("\nMission Accomplished: Development experience enhanced!");
    } else {
        eprintln!("🔥 Rust Observer: C generator reported an error (status code: {})!", status_code);
    }
}