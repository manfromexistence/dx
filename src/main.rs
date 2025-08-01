unsafe extern "C" {
    fn generate_magic_number(a: i32, b: i32) -> i32;
}

fn main() {
    println!("Rust Observer: Preparing to call the C generator...");

    let number1 = 10;
    let number2 = 32;

   let result = unsafe {
        generate_magic_number(number1, number2)
    };

    println!("Rust Observer: Received magic number ✨ -> {}", result);
    println!("\nMission Accomplished: Development experience enhanced!");
}