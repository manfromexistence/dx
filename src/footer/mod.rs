use std::{collections::HashMap, error::Error, fs::File, io::Read, path::PathBuf, str::from_utf8};

use rand::seq::SliceRandom;
use regex::Regex;
use rust_embed::RustEmbed;

// Change these to use `super::` as they are sibling modules within `footer`
use super::bubbles::{BubbleType, SpeechBubble};
use super::errors::CustomError; // Make sure to add this use for CustomError

#[derive(RustEmbed, Debug)]
// IMPORTANT: The path here is relative to the *crate root* (your Cargo.toml location)
// So, 'src/footer/charas' is the correct path for where your .chara files are located.
#[folder = "src/footer/charas"]
struct Asset;

/// Source chara to load, either builtin or from external file.
#[derive(Debug)]
pub enum Chara {
    All,
    Builtin(String),
    File(PathBuf),
    Raw(String),
    Random,
}

/// All built-in characters name.
pub const BUILTIN_CHARA: [&str; 24] = [
    "aya",
    "chocobo",
    "cirno",
    "clefairy",
    "cow",
    "eevee",
    "ferris",
    "ferris1",
    "flareon",
    "goldeen",
    "growlithe",
    "kirby",
    "kitten",
    "mario",
    "mew",
    "nemo",
    "pikachu",
    "piplup",
    "psyduck",
    "remilia-scarlet",
    "seaking",
    "togepi",
    "tux",
    "wartortle",
];

// ... (rest of your load_raw_chara_string, strip_chara_string, parse_character functions)

fn load_raw_chara_string(chara: &Chara) -> String {
    let mut raw_chara = String::new();

    match chara {
        Chara::File(s) => {
            // Using `eprintln` for errors in examples for simplicity,
            // but in a real app, you'd want proper error propagation.
            let mut file = File::open(s).unwrap_or_else(|err| {
                eprintln!("ERROR: Failed to open character file: {:#?}", err);
                std::process::exit(1); // Exit if file cannot be opened
            });
            file.read_to_string(&mut raw_chara)
                .unwrap_or_else(|err| {
                    eprintln!("ERROR: Failed to read character file: {:#?}", err);
                    std::process::exit(1); // Exit if file cannot be read
                });
        }

        Chara::Builtin(s) => {
            let name = format!("{}.chara", s);
            let asset = Asset::get(&name).unwrap_or_else(|| {
                eprintln!("ERROR: Built-in character '{}' not found.", s);
                std::process::exit(1); // Exit if asset not found
            });
            raw_chara = from_utf8(&asset.data)
                .unwrap_or_else(|err| {
                    eprintln!("ERROR: UTF-8 error decoding built-in character: {:#?}", err);
                    std::process::exit(1); // Exit if UTF-8 decoding fails
                })
                .to_string();
        }

        Chara::Raw(s) => {
            raw_chara = s.to_string();
        }

        Chara::All => {
            let charas = Asset::iter()
                .map(|file| {
                    let name = file.trim_end_matches(".chara");
                    let asset = Asset::get(&file).unwrap();
                    format!("{} 👇\n{}", name, String::from_utf8_lossy(&asset.data))
                })
                .collect::<Vec<_>>();
            raw_chara = charas.join("\n+\n");
        }

        Chara::Random => {
            let charas = Asset::iter().collect::<Vec<_>>();
            let choosen_chara = charas.choose(&mut rand::thread_rng()).unwrap_or_else(|| {
                eprintln!("ERROR: No built-in characters found for random selection.");
                std::process::exit(1); // Exit if no characters are available
            }).clone();
            let asset = Asset::get(&choosen_chara).unwrap();
            raw_chara = from_utf8(&asset.data)
                .unwrap_or_else(|err| {
                    eprintln!("ERROR: UTF-8 error decoding random character: {:#?}", err);
                    std::process::exit(1); // Exit if UTF-8 decoding fails
                })
                .to_string();
        }
    }

    raw_chara
}

fn strip_chara_string(raw_chara: &str) -> String {
    raw_chara
        .split('\n')
        .filter(|line| {
            !line.starts_with('#')
                && !line.starts_with("$x")
                && !line.contains("$thoughts")
                && !line.is_empty()
        })
        .collect::<Vec<_>>()
        .join("\n")
        .replace("\\e", "\x1B")
}

fn parse_character(chara: &Chara, voice_line: &str) -> String {
    let raw_chara = load_raw_chara_string(chara);
    let stripped_chara = strip_chara_string(&raw_chara);
    let charas = stripped_chara.split('+').collect::<Vec<_>>();
    let mut parsed = String::new();

    let re = Regex::new(r"(?<var>\$\w).*=.*(?<val>\x1B\[.*m\s*).;").unwrap();
    for chara in charas {
        // extract variable definition to HashMap
        let replacers: Vec<HashMap<&str, &str>> = re
            .captures_iter(chara)
            .map(|cap| {
                re.capture_names()
                    .flatten()
                    .filter_map(|n| Some((n, cap.name(n)?.as_str())))
                    .collect()
            })
            .collect();

        let mut chara_body = chara
            .split('\n')
            .filter(|line| !line.contains('=') && !line.contains("EOC"))
            .collect::<Vec<_>>()
            .join("\n")
            .trim_end()
            .replace("$x", "\x1B[49m  ")
            .replace("$t", voice_line);

        // replace variable from character's body with actual value
        for replacer in replacers {
            chara_body = chara_body.replace(
                replacer.get("var").copied().unwrap(),
                replacer.get("val").copied().unwrap(),
            );
        }

        parsed.push_str(&format!("{}\n\n\n", &chara_body))
    }

    parsed.trim_end().to_string()
}

/// Format arguments to form complete charasay
pub fn format_character(
    messages: &str,
    chara: &Chara,
    max_width: usize,
    bubble_type: BubbleType,
) -> Result<String, Box<dyn Error>> {
    let voice_line: &str;
    let bubble_type = match bubble_type {
        BubbleType::Think => {
            voice_line = "o ";
            BubbleType::Think
        }
        BubbleType::Round => {
            voice_line = "╲ ";
            BubbleType::Round
        }
        BubbleType::Cowsay => {
            voice_line = "\\ ";
            BubbleType::Cowsay
        }
        BubbleType::Ascii => {
            voice_line = "\\ ";
            BubbleType::Ascii
        }
        BubbleType::Unicode => {
            voice_line = "╲ ";
            BubbleType::Unicode
        }
    };

    let speech_bubble = SpeechBubble::new(bubble_type);
    let speech = speech_bubble.create(messages, &max_width)?;
    let character = parse_character(chara, voice_line);

    Ok(format!("{}{}", speech, character))
}

/// Print only the character
pub fn print_character(chara: &Chara) -> String {
    parse_character(chara, "  ")
}