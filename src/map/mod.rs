//! Map parsing, dynamic tileset resolution, UV calculation, and merged collision generation.

use glam::Vec2;
use std::path::Path;
use crate::engine::{Texture, WgpuRenderer};
use crate::physics::AABB;

pub mod parser;
pub use parser::*;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum InteractiveKind {
    SpawnPoint,
    Checkpoint,
    LevelExit,
    Light,
    Moon,
}

pub struct InteractiveTile {
    pub pos: Vec2,
    pub size: Vec2,
    pub kind: InteractiveKind,
    pub activated: bool,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TilesetKind {
    Bricks,
    Prototype,
    Main,
}

#[derive(Debug, Clone, PartialEq)]
pub struct TilesetMapping {
    pub firstgid: u32,
    pub kind: TilesetKind,
    pub cols: f32,
    pub rows: f32,
}

pub struct TileVisual {
    pub pos: Vec2,
    pub rot_deg: f32,
    pub scale: Vec2,
    pub uv_min: Vec2,
    pub uv_max: Vec2,
    pub tileset: TilesetKind,
}

pub struct TmxMap {
    pub width: u32,
    pub height: u32,
    pub tile_size: f32,
    pub initial_spawn_pos: Vec2,
    pub visuals: Vec<TileVisual>,
    pub solid_colliders: Vec<AABB>,
    pub one_way_platforms: Vec<AABB>,
    pub ice_colliders: Vec<AABB>,
    pub void_colliders: Vec<AABB>,
    pub spike_colliders: Vec<AABB>,
    pub interactives: Vec<InteractiveTile>,
    pub bricks_texture: Texture,
    pub prototype_texture: Texture,
    pub main_texture: Texture,
}

impl TmxMap {
    //------------[Load TMX map from file]-------------------
    pub fn load_from_file<P: AsRef<Path>>(
        path: P,
        renderer: &WgpuRenderer,
    ) -> Result<Self, String> {
        let content = std::fs::read_to_string(&path)
            .map_err(|e| format!("Failed to read TMX file {}: {}", path.as_ref().display(), e))?;

        let width = extract_attribute(&content, "width")
            .and_then(|s| s.parse::<u32>().ok())
            .unwrap_or(50);
        let height = extract_attribute(&content, "height")
            .and_then(|s| s.parse::<u32>().ok())
            .unwrap_or(50);
        let tile_size = extract_attribute(&content, "tilewidth")
            .and_then(|s| s.parse::<f32>().ok())
            .unwrap_or(32.0);

        let tileset_mappings = extract_tileset_mappings(&content);
        let layers = extract_all_layers(&content);

        let mut visuals = Vec::new();
        let mut solid_tile_coords = Vec::new();
        let mut one_way_tile_coords = Vec::new();
        let mut ice_tile_coords = Vec::new();
        let mut void_colliders = Vec::new();
        let mut spike_colliders = Vec::new();
        let mut interactives = Vec::new();
        let mut initial_spawn_pos = Vec2::new(100.0, 100.0);

        // Process CSV layers
        for (_layer_name, layer_csv) in &layers {
            let raw_gids = parse_raw_csv_gids(layer_csv);
            for (idx, &raw) in raw_gids.iter().enumerate() {
                if raw == 0 {
                    continue;
                }
                let (gid, rot_deg, scale) = decode_tile_gid(raw);
                let col = idx as u32 % width;
                let row = idx as u32 / width;
                let x = col as f32 * tile_size;
                let y = row as f32 * tile_size;
                let pos = Vec2::new(x, y);
                let size = Vec2::new(tile_size, tile_size);

                if let Some((mapping, local_id)) = resolve_tile(gid, &tileset_mappings) {
                    let (u0, u1, v0, v1) = calc_tile_uv(local_id, mapping.cols, mapping.rows);

                    // Skip static visual for collectible Moon to prevent duplicate rendering
                    if !(mapping.kind == TilesetKind::Main && local_id == 1) {
                        visuals.push(TileVisual {
                            pos,
                            rot_deg,
                            scale,
                            uv_min: Vec2::new(u0, v0),
                            uv_max: Vec2::new(u1, v1),
                            tileset: mapping.kind,
                        });
                    }

                    if mapping.kind == TilesetKind::Main {
                        match local_id {
                            0 => {
                                initial_spawn_pos = Vec2::new(x, y - 8.0);
                                interactives.push(InteractiveTile {
                                    pos,
                                    size,
                                    kind: InteractiveKind::SpawnPoint,
                                    activated: true,
                                });
                            }
                            1 => {
                                interactives.push(InteractiveTile {
                                    pos,
                                    size,
                                    kind: InteractiveKind::Moon,
                                    activated: false,
                                });
                            }
                            3 => {
                                interactives.push(InteractiveTile {
                                    pos,
                                    size,
                                    kind: InteractiveKind::Checkpoint,
                                    activated: false,
                                });
                            }
                            5 => {
                                interactives.push(InteractiveTile {
                                    pos,
                                    size,
                                    kind: InteractiveKind::LevelExit,
                                    activated: false,
                                });
                            }
                            6 => {
                                interactives.push(InteractiveTile {
                                    pos,
                                    size,
                                    kind: InteractiveKind::Light,
                                    activated: true,
                                });
                            }
                            8 => {
                                solid_tile_coords.push((col, row));
                            }
                            9 => {
                                one_way_tile_coords.push((col, row));
                            }
                            11 => {
                                // Ice tile acts as a solid collider (preventing seams) and registers as ice
                                solid_tile_coords.push((col, row));
                                ice_tile_coords.push((col, row));
                            }
                            12 => {
                                let void_box = AABB::from_pos_size(
                                    pos + Vec2::new(2.0, 2.0),
                                    size - Vec2::new(4.0, 4.0),
                                );
                                void_colliders.push(void_box);
                            }
                            13 => {
                                spike_colliders.push(calculate_spike_aabb(pos, rot_deg, scale));
                            }
                            _ => {}
                        }
                    }
                }
            }
        }

        // Merge horizontal colliders in the same row
        let solid_colliders = merge_horizontal_colliders(&solid_tile_coords, tile_size);
        let one_way_platforms = merge_horizontal_colliders(&one_way_tile_coords, tile_size);
        let ice_colliders = merge_horizontal_colliders(&ice_tile_coords, tile_size);

        // Fallback spawn check in object groups
        if interactives.iter().all(|i| i.kind != InteractiveKind::SpawnPoint) {
            if let Some(obj_spawn) = extract_object_spawn(&content) {
                initial_spawn_pos = obj_spawn;
            }
        }

        // Fallback visuals from individual tile coordinates if visual layer was empty
        if visuals.is_empty() && !solid_tile_coords.is_empty() {
            let (u0, u1, v0, v1) = calc_tile_uv(8, 8.0, 8.0);
            for &(col, row) in &solid_tile_coords {
                visuals.push(TileVisual {
                    pos: Vec2::new(col as f32 * tile_size, row as f32 * tile_size),
                    rot_deg: 0.0,
                    scale: Vec2::ONE,
                    uv_min: Vec2::new(u0, v0),
                    uv_max: Vec2::new(u1, v1),
                    tileset: TilesetKind::Main,
                });
            }
        }

        let bricks_texture = Texture::from_file(
            &renderer.device,
            &renderer.queue,
            "assets/tilesets/ts_bricks.png",
            &renderer.texture_bind_group_layout,
        )?;

        let prototype_texture = Texture::from_file(
            &renderer.device,
            &renderer.queue,
            "assets/tilesets/ts_prototype.png",
            &renderer.texture_bind_group_layout,
        )?;

        let main_texture = Texture::from_file(
            &renderer.device,
            &renderer.queue,
            "assets/tilesets/ts_main.png",
            &renderer.texture_bind_group_layout,
        )?;

        Ok(Self {
            width,
            height,
            tile_size,
            initial_spawn_pos,
            visuals,
            solid_colliders,
            one_way_platforms,
            ice_colliders,
            void_colliders,
            spike_colliders,
            interactives,
            bricks_texture,
            prototype_texture,
            main_texture,
        })
    }

    //------------[Render visible map tiles]-------------------
    pub fn render(&self, renderer: &mut WgpuRenderer, cam_pos: Vec2, viewport: Vec2, show_debug_tiles: bool) {
        let view_box = AABB::from_pos_size(
            cam_pos - Vec2::splat(self.tile_size * 2.0),
            viewport + Vec2::splat(self.tile_size * 4.0),
        );
        let tile_vec = Vec2::new(self.tile_size, self.tile_size);

        for tile in &self.visuals {
            // Functional tiles (ts_main) are hidden unless debug is on
            if tile.tileset == TilesetKind::Main && !show_debug_tiles {
                continue;
            }

            let tile_box = AABB::from_pos_size(tile.pos, tile_vec);
            if view_box.overlaps(&tile_box) {
                let (tex, tint) = match tile.tileset {
                    TilesetKind::Bricks => (&self.bricks_texture, [1.0, 1.0, 1.0, 1.0]),
                    TilesetKind::Prototype => (&self.prototype_texture, [1.0, 1.0, 1.0, 1.0]),
                    TilesetKind::Main => (&self.main_texture, [1.0, 1.0, 1.0, 0.85]),
                };
                renderer.draw_texture_rect_rotated(
                    tile.pos,
                    tile_vec,
                    tile.rot_deg,
                    tile.scale,
                    tile.uv_min,
                    tile.uv_max,
                    tex.id,
                    &tex.bind_group,
                    tint,
                );
            }
        }

        // Render unactivated Moon collectible items
        let (moon_u0, moon_u1, moon_v0, moon_v1) = calc_tile_uv(1, 8.0, 8.0);
        for item in &self.interactives {
            if item.kind == InteractiveKind::Moon && !item.activated {
                let item_box = AABB::from_pos_size(item.pos, item.size);
                if view_box.overlaps(&item_box) {
                    renderer.draw_texture_rect_rotated(
                        item.pos,
                        item.size,
                        0.0,
                        Vec2::ONE,
                        Vec2::new(moon_u0, moon_v0),
                        Vec2::new(moon_u1, moon_v1),
                        self.main_texture.id,
                        &self.main_texture.bind_group,
                        [1.0, 1.0, 1.0, 1.0],
                    );
                }
            }
        }
    }
}

//------------[Merge contiguous horizontal tile colliders into single AABBs]-------------------
// Scans sorted tile coordinates and merges horizontally adjacent tiles in the same row
pub fn merge_horizontal_colliders(tiles: &[(u32, u32)], tile_size: f32) -> Vec<AABB> {
    if tiles.is_empty() {
        return Vec::new();
    }
    let mut sorted = tiles.to_vec();
    // Sort by row first, then column
    sorted.sort_unstable_by_key(|&(col, row)| (row, col));
    sorted.dedup();

    let mut merged = Vec::new();
    let mut current_row = sorted[0].1;
    let mut start_col = sorted[0].0;
    let mut end_col = sorted[0].0;

    for &(col, row) in &sorted[1..] {
        if row == current_row && col == end_col + 1 {
            // Contiguous tile in the same row extends the current collider
            end_col = col;
        } else {
            // Gap or row boundary reached: finish the current collider span
            let x = start_col as f32 * tile_size;
            let y = current_row as f32 * tile_size;
            let width = (end_col - start_col + 1) as f32 * tile_size;
            merged.push(AABB::from_pos_size(Vec2::new(x, y), Vec2::new(width, tile_size)));

            current_row = row;
            start_col = col;
            end_col = col;
        }
    }

    // Finish final span
    let x = start_col as f32 * tile_size;
    let y = current_row as f32 * tile_size;
    let width = (end_col - start_col + 1) as f32 * tile_size;
    merged.push(AABB::from_pos_size(Vec2::new(x, y), Vec2::new(width, tile_size)));

    merged
}

//------------[Calculate rotated spike AABB]-------------------
pub fn calculate_spike_aabb(pos: Vec2, rot_deg: f32, scale: Vec2) -> AABB {
    let corners = [
        Vec2::new(3.0, 16.0),
        Vec2::new(29.0, 16.0),
        Vec2::new(29.0, 32.0),
        Vec2::new(3.0, 32.0),
    ];
    let center = Vec2::new(16.0, 16.0);
    let rad = rot_deg.to_radians();
    let cos = rad.cos();
    let sin = rad.sin();

    let mut min = Vec2::new(f32::MAX, f32::MAX);
    let mut max = Vec2::new(f32::MIN, f32::MIN);

    for corner in corners {
        let centered = corner - center;
        let scaled = Vec2::new(centered.x * scale.x, centered.y * scale.y);
        let rotated = Vec2::new(
            scaled.x * cos - scaled.y * sin,
            scaled.x * sin + scaled.y * cos,
        );
        let transformed = rotated + center;
        min = min.min(transformed);
        max = max.max(transformed);
    }

    AABB::new(pos + min, pos + max)
}

#[cfg(test)]
mod tests;
