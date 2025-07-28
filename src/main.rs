// fn main(){
//     println!("Hello, manfromexistence!")
// }

// In src/main.rs

// This line tells Rust to look for and include the code from `src/header/mod.rs`.
mod header;

fn main() {
    println!("--- Default Render (Left Aligned) ---");
    // Directly call the simple render function.
    header::render("DX");

    println!("\n--- Customized Render (Centered) ---");
    // For custom options, first load the font.
    // .expect() is used for a quick example; in real code, you might use a match or if let.
    let font = header::DXCliFont::default().expect("Failed to load the default font.");
    
    // Use the builder pattern to create and customize the figure.
    if let Some(figure) = font.figure("DX-CLI") {
        let centered_figure = figure.align(header::Alignment::Center);
        println!("{}", centered_figure);
    }

    println!("\n--- Customized Render (Right Aligned) ---");
    if let Some(figure) = font.figure("Rust Forge") {
        let right_aligned_figure = figure.align(header::Alignment::Right);
        println!("{}", right_aligned_figure);
    }
}
