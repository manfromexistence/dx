// fn main(){
//     println!("Hello, manfromexistence!")
// }

mod header;
use header::DXCliFont;

fn main() {
    match DXCliFont::default() {
        Ok(font) => {
            if let Some(figure) = font.convert("dx") {
                println!("{}", figure);
            }
        }
        Err(e) => {
            eprintln!("Error loading font: {}", e);
        }
    }
}
