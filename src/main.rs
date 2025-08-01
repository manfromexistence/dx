use std::time::Instant;

unsafe extern "C" {
    fn run_file_generator() -> i32;
}

fn main() {
    println!("🦀 Rust Observer: Initiating C file generator...");

    let start_time = Instant::now();

    let status_code = unsafe {
        run_file_generator()
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