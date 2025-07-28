// fn main(){
//     println!("Hello, manfromexistence!")
// }

mod header;

fn main() {
    println!("--- Default Render (Left Aligned) ---");
    header::render("What do you mean? I really wanna know! What do you mean!!");

    println!("\n--- Customized Render (Centered) ---");
    let font = header::DXCliFont::default().expect("Failed to load the default font.");

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
