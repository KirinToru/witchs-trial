use glam::Vec2;
use super::{TilesetKind, TilesetMapping};

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
        let after_attr = abs_start + attr.len();
        if xml[after_attr..].starts_with("=\"") {
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

//------------[Extract CSV data by layer name]-------------------
#[allow(dead_code)]
pub(crate) fn extract_layer_data(xml: &str, layer_name: &str) -> Option<String> {
    let search = format!("name=\"{layer_name}\"");
    let layer_start = xml.find(&search)?;
    let data_start = xml[layer_start..].find("<data")? + layer_start;
    let data_open = xml[data_start..].find('>')? + data_start + 1;
    let data_close = xml[data_open..].find("</data>")? + data_open;
    Some(xml[data_open..data_close].to_string())
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

