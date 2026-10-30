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
    pub hazard_colliders: Vec<AABB>,
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
        let mut solid_colliders = Vec::new();
        let mut one_way_platforms = Vec::new();
        let mut hazard_colliders = Vec::new();
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
                let x = (idx as u32 % width) as f32 * tile_size;
                let y = (idx as u32 / width) as f32 * tile_size;
                let pos = Vec2::new(x, y);
                let size = Vec2::new(tile_size, tile_size);

                if let Some((mapping, local_id)) = resolve_tile(gid, &tileset_mappings) {
                    let (u0, u1, v0, v1) = calc_tile_uv(local_id, mapping.cols, mapping.rows);

                    visuals.push(TileVisual {
                        pos,
                        rot_deg,
                        scale,
                        uv_min: Vec2::new(u0, v0),
                        uv_max: Vec2::new(u1, v1),
                        tileset: mapping.kind,
                    });

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
                                solid_colliders.push(AABB::from_pos_size(pos, size));
                            }
                            9 => {
                                one_way_platforms.push(AABB::from_pos_size(pos, size));
                            }
                            12 => {
                                let void_box = AABB::from_pos_size(
                                    pos + Vec2::new(2.0, 2.0),
                                    size - Vec2::new(4.0, 4.0),
                                );
                                hazard_colliders.push(void_box);
                            }
                            13 => {
                                hazard_colliders.push(calculate_spike_aabb(pos, rot_deg, scale));
                            }
                            _ => {}
                        }
                    }
                }
            }
        }

        // Fallback spawn check in object groups
        if interactives.iter().all(|i| i.kind != InteractiveKind::SpawnPoint) {
            if let Some(obj_spawn) = extract_object_spawn(&content) {
                initial_spawn_pos = obj_spawn;
            }
        }

        // Fallback visuals from colliders if visual layer was empty
        if visuals.is_empty() && !solid_colliders.is_empty() {
            let (u0, u1, v0, v1) = calc_tile_uv(8, 8.0, 8.0);
            for col in &solid_colliders {
                visuals.push(TileVisual {
                    pos: col.min,
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
            hazard_colliders,
            interactives,
            bricks_texture,
            prototype_texture,
            main_texture,
        })
    }

    //------------[Reload tileset textures]-------------------
    #[allow(dead_code)]
    pub fn reload_textures(&mut self, renderer: &WgpuRenderer) -> Result<(), String> {
        self.bricks_texture = Texture::from_file(
            &renderer.device,
            &renderer.queue,
            "assets/tilesets/ts_bricks.png",
            &renderer.texture_bind_group_layout,
        )?;
        self.prototype_texture = Texture::from_file(
            &renderer.device,
            &renderer.queue,
            "assets/tilesets/ts_prototype.png",
            &renderer.texture_bind_group_layout,
        )?;
        self.main_texture = Texture::from_file(
            &renderer.device,
            &renderer.queue,
            "assets/tilesets/ts_main.png",
            &renderer.texture_bind_group_layout,
        )?;
        Ok(())
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
    }
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
