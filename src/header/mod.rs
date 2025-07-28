use std::collections::HashMap;
use std::fmt;

pub struct DXCliFont {
    pub header_line: HeaderLine,
    pub fonts: HashMap<u32, DXCliFontCharacter>,
}

impl DXCliFont {
    fn read_header_line(header_line: &str) -> Result<HeaderLine, String> {
        HeaderLine::try_from(header_line)
    }

    fn extract_one_line(
        lines: &[&str],
        index: usize,
        height: usize,
        hardblank: char,
        is_last_index: bool,
    ) -> Result<String, String> {
        let line = lines
            .get(index)
            .ok_or(format!("can't get line at specified index:{index}"))?;

        let mut width = line.len() - 1;
        if is_last_index && height != 1 {
            width -= 1;
        }

        Ok(line[..width].replace(hardblank, " "))
    }

    fn extract_one_font(
        lines: &[&str],
        start_index: usize,
        height: usize,
        hardblank: char,
    ) -> Result<DXCliFontCharacter, String> {
        let mut characters = vec![];
        for i in 0..height {
            let index = start_index + i;
            let is_last_index = i == height - 1;
            let one_line_character =
                DXCliFont::extract_one_line(lines, index, height, hardblank, is_last_index)?;
            characters.push(one_line_character);
        }

        Ok(DXCliFontCharacter { characters })
    }

    fn read_required_font(
        lines: &[&str],
        headerline: &HeaderLine,
        map: &mut HashMap<u32, DXCliFontCharacter>,
    ) -> Result<(), String> {
        let offset = (1 + headerline.comment_lines) as usize;
        let height = headerline.height as usize;
        let size = lines.len();

        for i in 0..=94 {
            let code = (i + 32) as u32;
            let start_index = offset + i * height;
            if start_index >= size {
                break;
            }

            let font =
                DXCliFont::extract_one_font(lines, start_index, height, headerline.hardblank)?;
            map.insert(code, font);
        }

        let offset = offset + 95 * height;
        let required_deutsch_characters_codes: [u32; 7] = [196, 214, 220, 228, 246, 252, 223];
        for (i, code) in required_deutsch_characters_codes.iter().enumerate() {
            let start_index = offset + i * height;
            if start_index >= size {
                break;
            }

            let font =
                DXCliFont::extract_one_font(lines, start_index, height, headerline.hardblank)?;
            map.insert(*code, font);
        }

        Ok(())
    }

    fn extract_codetag_font_code(lines: &[&str], index: usize) -> Result<u32, String> {
        let line = lines
            .get(index)
            .ok_or_else(|| "get codetag line error".to_string())?;

        let infos: Vec<&str> = line.trim().split(' ').collect();
        if infos.is_empty() {
            return Err("extract code for codetag font error".to_string());
        }

        let code = infos[0].trim();

        let code = if let Some(s) = code.strip_prefix("0x") {
            u32::from_str_radix(s, 16)
        } else if let Some(s) = code.strip_prefix("0X") {
            u32::from_str_radix(s, 16)
        } else if let Some(s) = code.strip_prefix('0') {
            u32::from_str_radix(s, 8)
        } else {
            code.parse()
        };

        code.map_err(|e| format!("{e:?}"))
    }

    fn read_codetag_font(
        lines: &[&str],
        headerline: &HeaderLine,
        map: &mut HashMap<u32, DXCliFontCharacter>,
    ) -> Result<(), String> {
        let offset = (1 + headerline.comment_lines + 102 * headerline.height) as usize;
        let codetag_height = (headerline.height + 1) as usize;
        let codetag_lines = lines.len() - offset;

        if codetag_lines % codetag_height != 0 {
            return Err("codetag font is illegal.".to_string());
        }

        let size = codetag_lines / codetag_height;

        for i in 0..size {
            let start_index = offset + i * codetag_height;
            if start_index >= lines.len() {
                break;
            }

            let code = DXCliFont::extract_codetag_font_code(lines, start_index)?;
            let font = DXCliFont::extract_one_font(
                lines,
                start_index + 1,
                headerline.height as usize,
                headerline.hardblank,
            )?;
            map.insert(code, font);
        }

        Ok(())
    }

    fn read_fonts(
        lines: &[&str],
        headerline: &HeaderLine,
    ) -> Result<HashMap<u32, DXCliFontCharacter>, String> {
        let mut map = HashMap::new();
        DXCliFont::read_required_font(lines, headerline, &mut map)?;
        DXCliFont::read_codetag_font(lines, headerline, &mut map)?;
        Ok(map)
    }

    pub fn from_content(contents: &str) -> Result<DXCliFont, String> {
        let lines: Vec<&str> = contents.lines().collect();

        if lines.is_empty() {
            return Err("can not generate DX-CLI-Font from empty string".to_string());
        }

        let header_line = DXCliFont::read_header_line(lines.first().unwrap())?;
        let fonts = DXCliFont::read_fonts(&lines, &header_line)?;

        Ok(DXCliFont { header_line, fonts })
    }

    pub fn default() -> Result<DXCliFont, String> {
        let contents = std::include_str!("default.dxcf");
        DXCliFont::from_content(contents)
    }

    pub fn convert(&self, message: &str) -> Option<DXCliFigure> {
        if message.is_empty() {
            return None;
        }

        let mut characters: Vec<&DXCliFontCharacter> = vec![];
        for ch in message.chars() {
            let code = ch as u32;
            if let Some(character) = self.fonts.get(&code) {
                characters.push(character);
            }
        }

        if characters.is_empty() {
            return None;
        }

        Some(DXCliFigure {
            characters,
            height: self.header_line.height as u32,
        })
    }
}

pub struct HeaderLine {
    pub hardblank: char,
    pub height: i32,
    pub comment_lines: i32,
}

impl HeaderLine {
    fn extract_required_info(infos: &[&str], index: usize, field: &str) -> Result<i32, String> {
        let val = match infos.get(index) {
            Some(val) => Ok(val),
            None => Err(format!(
                "can't get field:{field} index:{index} from {}",
                infos.join(",")
            )),
        }?;

        val.parse()
            .map_err(|_| format!("can't parse required field:{field} of {val} to i32"))
    }
}

impl TryFrom<&str> for HeaderLine {
    type Error = String;

    fn try_from(header_line: &str) -> Result<Self, Self::Error> {
        let infos: Vec<&str> = header_line.trim().split(' ').collect();

        if infos.len() < 6 {
            return Err("headerline is illegal".to_string());
        }

        let signature_with_hardblank = infos
            .first()
            .ok_or("Can't get signature from header".to_string())?;

        let hardblank = signature_with_hardblank
            .chars()
            .last()
            .ok_or("Can't get hardblank from header".to_string())?;

        let height = HeaderLine::extract_required_info(&infos, 1, "height")?;
        let comment_lines = HeaderLine::extract_required_info(&infos, 5, "comment lines")?;

        Ok(HeaderLine {
            hardblank,
            height,
            comment_lines,
        })
    }
}

pub struct DXCliFontCharacter {
    pub characters: Vec<String>,
}

impl fmt::Display for DXCliFontCharacter {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(f, "{}", self.characters.join("\n"))
    }
}

pub struct DXCliFigure<'a> {
    pub characters: Vec<&'a DXCliFontCharacter>,
    pub height: u32,
}

impl<'a> DXCliFigure<'a> {
    fn is_not_empty(&self) -> bool {
        !self.characters.is_empty() && self.height > 0
    }
}

impl<'a> fmt::Display for DXCliFigure<'a> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        if self.is_not_empty() {
            let mut rs: Vec<&'a str> = vec![];
            for i in 0..self.height {
                for character in &self.characters {
                    if let Some(line) = character.characters.get(i as usize) {
                        rs.push(line);
                    }
                }
                rs.push("\n");
            }

            write!(f, "{}", rs.join(""))
        } else {
            write!(f, "")
        }
    }
}