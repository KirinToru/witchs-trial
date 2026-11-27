use glam::Vec2;
use super::{NoteObject, TileVisual, TilesetKind, TilesetMapping};

pub const FLIP_H: u64 = 0x8000_0000;
pub const FLIP_V: u64 = 0x4000_0000;
pub const FLIP_D: u64 = 0x2000_0000;
pub const GID_MASK: u64 = 0x1FFF_FFFF;

//------------[Compute subtile UV coordinates]-------------------
pub fn calc_tile_uv(local_id: u32, cols: f32, rows: f32) -> (f32, f32, f32, f32) {
    let col = (local_id as f32 % cols).floor();
    let row = (local_id as f32 / cols).floor();
    (
        col / cols,
        (col + 1.0) / cols,
        row / rows,
        (row + 1.0) / rows,
    )
}

//------------[Decode tile GID and orientation flags]-------------------
pub fn decode_tile_gid(raw: u64) -> (u32, f32, Vec2) {
    let flip_h = (raw & FLIP_H) != 0;
    let flip_v = (raw & FLIP_V) != 0;
    let flip_d = (raw & FLIP_D) != 0;
    let gid = (raw & GID_MASK) as u32;

    let (rot_deg, sx, sy) = match (flip_d, flip_h, flip_v) {
        (false, false, false) => (0.0, 1.0, 1.0),
        (false, true, false)  => (0.0, -1.0, 1.0),
        (false, false, true)  => (0.0, 1.0, -1.0),
        (false, true, true)   => (180.0, 1.0, 1.0),
        (true, false, false)  => (270.0, -1.0, 1.0),
        (true, true, false)   => (90.0, 1.0, 1.0),
        (true, false, true)   => (270.0, 1.0, 1.0),
        (true, true, true)    => (90.0, -1.0, 1.0),
    };

    (gid, rot_deg, Vec2::new(sx, sy))
}

//------------[Extract attribute from XML string]-------------------
pub fn extract_attribute<'a>(xml: &'a str, attr: &str) -> Option<&'a str> {
    let mut cursor = 0;
    while let Some(idx) = xml[cursor..].find(attr) {
        let abs_start = cursor + idx;
        let at_boundary = abs_start == 0
            || xml[..abs_start]
                .chars()
                .next_back()
                .is_some_and(|c| c.is_ascii_whitespace());
        let after_attr = abs_start + attr.len();
        if at_boundary && xml[after_attr..].starts_with("=\"") {
            let val_start = after_attr + 2;
            let val_end = xml[val_start..].find('\"')? + val_start;
            return Some(&xml[val_start..val_end]);
        }
        cursor = after_attr;
    }
    None
}

//------------[Extract dynamic tileset mappings from TMX]-------------------
pub fn extract_tileset_mappings(xml: &str) -> Vec<TilesetMapping> {
    let mut mappings = Vec::new();
    let mut cursor = 0;
    while let Some(idx) = xml[cursor..].find("<tileset") {
        let abs_start = cursor + idx;
        let tag_end = match xml[abs_start..].find('>') {
            Some(e) => abs_start + e + 1,
            None => break,
        };
        let tag = &xml[abs_start..tag_end];
        let firstgid = extract_attribute(tag, "firstgid")
            .and_then(|s| s.parse::<u32>().ok())
            .unwrap_or(1);
        let source_or_name = extract_attribute(tag, "source")
            .or_else(|| extract_attribute(tag, "name"))
            .unwrap_or_default()
            .to_lowercase();

        let (kind, cols, rows) = if source_or_name.contains("bricks") {
            (TilesetKind::Bricks, 3.0, 2.0)
        } else if source_or_name.contains("proto") {
            (TilesetKind::Prototype, 8.0, 4.0)
        } else {
            (TilesetKind::Main, 8.0, 8.0)
        };

        mappings.push(TilesetMapping {
            firstgid,
            kind,
            cols,
            rows,
        });

        cursor = tag_end;
    }

    if mappings.is_empty() {
        // Fallback default mappings for synthetic maps
        mappings.push(TilesetMapping {
            firstgid: 1,
            kind: TilesetKind::Bricks,
            cols: 3.0,
            rows: 2.0,
        });
        mappings.push(TilesetMapping {
            firstgid: 7,
            kind: TilesetKind::Prototype,
            cols: 8.0,
            rows: 4.0,
        });
        mappings.push(TilesetMapping {
            firstgid: 39,
            kind: TilesetKind::Main,
            cols: 8.0,
            rows: 8.0,
        });
    }

    mappings.sort_by_key(|a| std::cmp::Reverse(a.firstgid));
    mappings
}

//------------[Resolve tile GID to local subtile ID]-------------------
pub fn resolve_tile(gid: u32, mappings: &[TilesetMapping]) -> Option<(&TilesetMapping, u32)> {
    for mapping in mappings {
        if gid >= mapping.firstgid {
            let local_id = gid - mapping.firstgid;
            return Some((mapping, local_id));
        }
    }
    None
}

//------------[Extract ordered CSV layers from TMX]-------------------
pub fn extract_all_layers(xml: &str) -> Vec<(String, String)> {
    let mut layers = Vec::new();
    let mut cursor = 0;
    while let Some(layer_idx) = xml[cursor..].find("<layer") {
        let abs_layer = cursor + layer_idx;
        let layer_end = match xml[abs_layer..].find("</layer>") {
            Some(e) => abs_layer + e + "</layer>".len(),
            None => break,
        };
        let layer_chunk = &xml[abs_layer..layer_end];
        let name = extract_attribute(layer_chunk, "name").unwrap_or("unnamed").to_string();
        if let Some(data_start_rel) = layer_chunk.find("<data") {
            if let Some(data_open_rel) = layer_chunk[data_start_rel..].find('>') {
                let abs_data_open = data_start_rel + data_open_rel + 1;
                if let Some(data_close_rel) = layer_chunk[abs_data_open..].find("</data>") {
                    let abs_data_close = abs_data_open + data_close_rel;
                    let data_csv = layer_chunk[abs_data_open..abs_data_close].trim().to_string();
                    layers.push((name, data_csv));
                }
            }
        }
        cursor = layer_end;
    }
    layers
}

//------------[Extract spawn position from object groups]-------------------
pub fn extract_object_spawn(xml: &str) -> Option<Vec2> {
    let mut cursor = 0;
    while let Some(idx) = xml[cursor..].find("<object") {
        let abs_start = cursor + idx;
        let tag_end = match xml[abs_start..].find('>') {
            Some(e) => abs_start + e + 1,
            None => break,
        };
        let tag = &xml[abs_start..tag_end];
        let name = extract_attribute(tag, "name").unwrap_or_default().to_lowercase();
        if name.contains("spawn") || name.contains("start") {
            let x = extract_attribute(tag, "x").and_then(|s| s.parse::<f32>().ok())?;
            let y = extract_attribute(tag, "y").and_then(|s| s.parse::<f32>().ok())?;
            return Some(Vec2::new(x, y));
        }
        cursor = tag_end;
    }
    None
}

//------------[Parse raw CSV GID string]-------------------
pub fn parse_raw_csv_gids(csv: &str) -> Vec<u64> {
    csv.split(',')
        .filter_map(|s| {
            let clean = s.trim();
            if clean.is_empty() {
                None
            } else {
                clean.parse::<u64>().ok()
            }
        })
        .collect()
}

//------------[Find next exact XML start tag ignoring longer tag names]-------------------
fn find_next_element(xml: &str, tag_name: &str, mut cursor: usize) -> Option<usize> {
    let pattern = format!("<{}", tag_name);
    while let Some(rel) = xml[cursor..].find(&pattern) {
        let abs = cursor + rel;
        let after = abs + pattern.len();
        let next_byte = xml.as_bytes().get(after).copied();
        // Ensure <object does not match <objectgroup and <property does not match <properties
        if matches!(next_byte, Some(b' ' | b'\t' | b'\n' | b'\r' | b'>' | b'/')) {
            return Some(abs);
        }
        cursor = after;
    }
    None
}

//------------[Find closing bracket of an XML opening tag outside quotes]-------------------
fn find_tag_end(xml: &str, start: usize) -> Option<usize> {
    let mut in_quotes = false;
    for (offset, ch) in xml[start..].char_indices() {
        match ch {
            '"' => in_quotes = !in_quotes,
            '>' if !in_quotes => return Some(start + offset + 1),
            _ => {}
        }
    }
    None
}

//------------[Decode XML character entities and escaped newlines]-------------------
fn decode_xml_text(raw: &str) -> String {
    let mut out = String::with_capacity(raw.len());
    let mut i = 0;
    while i < raw.len() {
        let rest = &raw[i..];
        if rest.starts_with("\r\n") {
            out.push('\n');
            i += 2;
        } else if rest.starts_with("&#13;&#10;") {
            out.push('\n');
            i += 10;
        } else if rest.starts_with("&#10;") {
            out.push('\n');
            i += 5;
        } else if rest.starts_with("&#13;") {
            i += 5;
        } else if rest.starts_with("&quot;") {
            out.push('"');
            i += 6;
        } else if rest.starts_with("&apos;") {
            out.push('\'');
            i += 6;
        } else if rest.starts_with("&lt;") {
            out.push('<');
            i += 4;
        } else if rest.starts_with("&gt;") {
            out.push('>');
            i += 4;
        } else if rest.starts_with("&amp;") {
            out.push('&');
            i += 5;
        } else if rest.starts_with("\\r\\n") {
            out.push('\n');
            i += 4;
        } else if rest.starts_with("\\n") {
            out.push('\n');
            i += 2;
        } else if let Some(ch) = rest.chars().next() {
            out.push(ch);
            i += ch.len_utf8();
        }
    }
    out
}

//------------[Extract title and text properties from an object XML body]-------------------
fn extract_note_properties(body: &str) -> (Option<String>, Option<String>) {
    let mut title = None;
    let mut text = None;
    let mut cursor = 0;

    while let Some(prop_start) = find_next_element(body, "property", cursor) {
        let Some(prop_tag_end) = find_tag_end(body, prop_start) else {
            break;
        };
        let prop_tag = &body[prop_start..prop_tag_end];
        let self_closing = prop_tag.trim_end_matches('>').trim_end().ends_with('/');

        let raw_val = if let Some(val) = extract_attribute(prop_tag, "value") {
            cursor = prop_tag_end;
            Some(val)
        } else if !self_closing {
            if let Some(close_rel) = body[prop_tag_end..].find("</property>") {
                let close_abs = prop_tag_end + close_rel;
                cursor = close_abs + "</property>".len();
                Some(&body[prop_tag_end..close_abs])
            } else {
                cursor = prop_tag_end;
                None
            }
        } else {
            cursor = prop_tag_end;
            None
        };

        if let Some(prop_name) = extract_attribute(prop_tag, "name") {
            if let Some(val) = raw_val {
                let decoded = decode_xml_text(val);
                if prop_name.eq_ignore_ascii_case("title") {
                    title = Some(decoded);
                } else if prop_name.eq_ignore_ascii_case("text") {
                    text = Some(decoded);
                }
            }
        }
    }

    (title, text)
}

//------------[Check whether a Tiled object represents an interactive note]-------------------
fn is_note_object(tag: &str, has_text_prop: bool) -> bool {
    if has_text_prop {
        return true;
    }
    for attr in ["type", "class"] {
        if let Some(val) = extract_attribute(tag, attr) {
            let trimmed = val.trim();
            if trimmed.eq_ignore_ascii_case("note")
                || trimmed.eq_ignore_ascii_case("sign")
                || trimmed.eq_ignore_ascii_case("instruction")
            {
                return true;
            }
        }
    }
    if let Some(name) = extract_attribute(tag, "name") {
        let lower = name.trim().to_ascii_lowercase();
        if lower.starts_with("note") || lower.starts_with("sign") {
            return true;
        }
    }
    false
}

//------------[Extract interactive note objects from TMX object groups]-------------------
pub fn extract_note_objects(
    xml: &str,
    tile_size: f32,
    tileset_mappings: &[TilesetMapping],
) -> Vec<NoteObject> {
    let default_size = if tile_size > 0.0 { tile_size } else { 32.0 };
    let mut notes = Vec::new();
    let mut cursor = 0;

    while let Some(obj_start) = find_next_element(xml, "object", cursor) {
        let Some(tag_end) = find_tag_end(xml, obj_start) else {
            break;
        };
        let open_tag = &xml[obj_start..tag_end];
        let self_closing = open_tag.trim_end_matches('>').trim_end().ends_with('/');

        let body = if self_closing {
            cursor = tag_end;
            ""
        } else if let Some(close_rel) = xml[tag_end..].find("</object>") {
            let close_abs = tag_end + close_rel;
            cursor = close_abs + "</object>".len();
            &xml[tag_end..close_abs]
        } else {
            cursor = tag_end;
            ""
        };

        let (title_opt, text_opt) = extract_note_properties(body);
        if !is_note_object(open_tag, text_opt.is_some()) {
            continue;
        }

        let x = extract_attribute(open_tag, "x")
            .and_then(|s| s.parse::<f32>().ok())
            .unwrap_or(0.0);
        let y = extract_attribute(open_tag, "y")
            .and_then(|s| s.parse::<f32>().ok())
            .unwrap_or(0.0);
        let width = extract_attribute(open_tag, "width")
            .and_then(|s| s.parse::<f32>().ok())
            .unwrap_or(default_size);
        let height = extract_attribute(open_tag, "height")
            .and_then(|s| s.parse::<f32>().ok())
            .unwrap_or(default_size);
        let size = Vec2::new(width, height);

        let raw_gid = extract_attribute(open_tag, "gid").and_then(|s| s.parse::<u64>().ok());
        // Tile Objects store bottom-left (x, y) in TMX; regular rectangles store top-left
        let (pos, visual) = if let Some(raw) = raw_gid.filter(|&g| g > 0) {
            let top_left = Vec2::new(x, y - height);
            let (gid, rot_deg, scale) = decode_tile_gid(raw);
            let tile_visual = resolve_tile(gid, tileset_mappings).map(|(mapping, local_id)| {
                let (u0, u1, v0, v1) = calc_tile_uv(local_id, mapping.cols, mapping.rows);
                TileVisual {
                    pos: top_left,
                    rot_deg,
                    scale,
                    uv_min: Vec2::new(u0, v0),
                    uv_max: Vec2::new(u1, v1),
                    tileset: mapping.kind,
                }
            });
            (top_left, tile_visual)
        } else {
            (Vec2::new(x, y), None)
        };

        notes.push(NoteObject {
            pos,
            size,
            title: title_opt.unwrap_or_default(),
            text: text_opt.unwrap_or_default(),
            visual,
        });
    }

    notes
}

//------------[Wrap note text across manual newlines and word boundaries]-------------------
pub fn wrap_note_text(text: &str, max_chars_per_line: usize) -> Vec<String> {
    if text.is_empty() {
        return Vec::new();
    }
    let limit = max_chars_per_line.max(1);
    let mut lines = Vec::new();

    for raw_para in text.split('\n') {
        let para = raw_para.trim_end_matches('\r');
        if para.trim().is_empty() {
            lines.push(String::new());
            continue;
        }

        let mut current = String::new();
        let mut current_len = 0usize;

        for word in para.split_whitespace() {
            let word_len = word.chars().count();
            if word_len > limit {
                if !current.is_empty() {
                    lines.push(std::mem::take(&mut current));
                    current_len = 0;
                }
                // Split extra-long tokens cleanly at exact character limits
                let mut chunk = String::new();
                let mut chunk_len = 0usize;
                for ch in word.chars() {
                    chunk.push(ch);
                    chunk_len += 1;
                    if chunk_len == limit {
                        lines.push(std::mem::take(&mut chunk));
                        chunk_len = 0;
                    }
                }
                if !chunk.is_empty() {
                    current = chunk;
                    current_len = chunk_len;
                }
            } else if current.is_empty() {
                current.push_str(word);
                current_len = word_len;
            } else if current_len + 1 + word_len <= limit {
                current.push(' ');
                current.push_str(word);
                current_len += 1 + word_len;
            } else {
                lines.push(std::mem::take(&mut current));
                current.push_str(word);
                current_len = word_len;
            }
        }

        if !current.is_empty() {
            lines.push(current);
        }
    }

    lines
}

