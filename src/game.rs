use glam::Vec2;
use crate::engine::{InputState, Key, Texture, WgpuRenderer};
use crate::map::TmxMap;
use crate::player::Player;

pub struct Game {
    pub map: TmxMap,
    pub player: Player,
    pub bg_texture: Texture,
    pub camera_pos: Vec2,
    pub viewport_size: Vec2,
    pub fade_alpha: f32,
    pub fading_out: bool,
    pub debug_mode: bool,
    pub fps: u32,
    pub frame_time_ms: f32,
    frame_counter: u32,
    fps_timer: f32,
    pub current_window_mode: String,
    pub step_hz: f32,
    pub level_time: f32,
    pub deaths_count: u32,
    pub level_finished: bool,
    pub active_backend_name: String,
    pub timestep_mode_name: String,
    pub auto_bhop: bool,
    pub r_hold_reset_done: bool,
    death_counted: bool,
}

impl Game {
    //------------[Create game instance]-------------------
    pub fn new(map_path: &str, renderer: &WgpuRenderer) -> Result<Self, String> {
        let map = TmxMap::load_from_file(map_path, renderer)?;
        let player = Player::new(map.initial_spawn_pos, renderer)?;
        let bg_texture = Texture::from_file(
            &renderer.device,
            &renderer.queue,
            "assets/backgrounds/bg.png",
            &renderer.texture_bind_group_layout,
        )?;

        // 640x360 virtual viewport (2x scale on 1280x720)
        let viewport_size = Vec2::new(640.0, 360.0);
        let mut game = Self {
            map,
            player,
            bg_texture,
            camera_pos: Vec2::ZERO,
            viewport_size,
            fade_alpha: 0.0,
            fading_out: false,
            debug_mode: false,
            fps: 60,
            frame_time_ms: 16.6,
            frame_counter: 0,
            fps_timer: 0.0,
            current_window_mode: "Windowed (1280x720)".to_string(),
            step_hz: 60.0,
            level_time: 0.0,
            deaths_count: 0,
            level_finished: false,
            active_backend_name: renderer.active_backend_name.clone(),
            timestep_mode_name: "Dynamic (Smooth)".to_string(),
            auto_bhop: true,
            r_hold_reset_done: false,
            death_counted: false,
        };
        game.snap_camera_to_player();
        Ok(game)
    }

    //------------[Center camera on player]-------------------
    pub fn snap_camera_to_player(&mut self) {
        let target = self.player.pos + self.player.size * 0.5 - self.viewport_size * 0.5;
        let max_cam = (Vec2::new(self.map.width as f32, self.map.height as f32) * self.map.tile_size - self.viewport_size).max(Vec2::ZERO);
        self.camera_pos = target.clamp(Vec2::ZERO, max_cam);
    }

    //------------[Reload graphics]-------------------
    #[allow(dead_code)]
    pub fn reload_graphics(&mut self, renderer: &WgpuRenderer) -> Result<(), String> {
        self.bg_texture = Texture::from_file(
            &renderer.device,
            &renderer.queue,
            "assets/backgrounds/bg.png",
            &renderer.texture_bind_group_layout,
        )?;
        self.map.reload_textures(renderer)?;
        self.player.reload_textures(renderer)?;
        self.active_backend_name = renderer.active_backend_name.clone();
        Ok(())
    }

    //------------[Full level restart]-------------------
    pub fn full_restart(&mut self) {
        self.player.reset(self.map.initial_spawn_pos);
        self.player.checkpoint_pos = self.map.initial_spawn_pos;
        self.snap_camera_to_player();
        self.fade_alpha = 0.0;
        self.fading_out = false;
        self.level_time = 0.0;
        self.deaths_count = 0;
        self.player.jumps_count = 0;
        self.player.dashes_count = 0;
        self.level_finished = false;
        self.r_hold_reset_done = false;
        self.death_counted = false;
    }

    //------------[Load map]-------------------
    pub fn change_map(&mut self, map_path: &str, renderer: &WgpuRenderer) -> Result<(), String> {
        self.map = TmxMap::load_from_file(map_path, renderer)?;
        self.full_restart();
        Ok(())
    }

    //------------[Respawn at checkpoint]-------------------
    pub fn reset_level(&mut self) {
        let respawn_pos = self.player.checkpoint_pos;
        self.player.reset(respawn_pos);
        self.snap_camera_to_player();
        self.death_counted = false;
    }

    //------------[Update FPS telemetry]-------------------
    pub fn record_render_frame(&mut self, dt: f32) {
        self.frame_counter += 1;
        self.fps_timer += dt;
        if self.fps_timer >= 0.5 {
            self.fps = (self.frame_counter as f32 / self.fps_timer).round() as u32;
            self.frame_time_ms = if self.fps > 0 { 1000.0 / self.fps as f32 } else { 0.0 };
            self.frame_counter = 0;
            self.fps_timer = 0.0;
        }
    }

    //------------[Update game state]-------------------
    pub fn update(&mut self, dt: f32, input: &InputState) {
        // F3 debug overlay toggle
        if input.is_just_pressed(Key::ToggleDebug) {
            self.debug_mode = !self.debug_mode;
        }

        // Level timer
        if !self.level_finished {
            self.level_time += dt;
        }

        // Bottom pit death
        let map_bottom = self.map.height as f32 * self.map.tile_size;
        if self.player.pos.y > map_bottom && !self.player.dead {
            self.player.dead = true;
        }

        // Death counter
        if self.player.dead && !self.death_counted {
            self.deaths_count += 1;
            self.death_counted = true;
        }

        // Level exit check
        let player_box = self.player.aabb();
        for item in &mut self.map.interactives {
            if item.kind == crate::map::InteractiveKind::LevelExit {
                let exit_box = crate::physics::AABB::from_pos_size(item.pos, item.size);
                if player_box.overlaps(&exit_box) {
                    item.activated = true;
                    self.level_finished = true;
                }
            }
        }

        // Hold R to restart: fade to black, reset on full black, fade back on release
        let holding_r = input.is_pressed(Key::Reset);
        if holding_r {
            self.fade_alpha = (self.fade_alpha + dt * 2.5).min(1.0);
            if self.fade_alpha >= 1.0 && !self.r_hold_reset_done {
                self.reset_level();
                self.r_hold_reset_done = true;
            }
        } else {
            self.r_hold_reset_done = false;
            if self.player.dead {
                self.fade_alpha = (self.fade_alpha + dt * 4.0).min(1.0);
                if self.fade_alpha >= 1.0 {
                    self.reset_level();
                }
            } else if self.fade_alpha > 0.0 {
                self.fade_alpha = (self.fade_alpha - dt * 3.0).max(0.0);
            }
        }

        // Simulation tick when not in blackout
        if self.fade_alpha < 0.95 && !self.level_finished {
            self.player.auto_bhop = self.auto_bhop;
            self.player.update(
                dt,
                input,
                &self.map.solid_colliders,
                &self.map.one_way_platforms,
                &self.map.hazard_colliders,
                &mut self.map.interactives,
            );
        }

        // Smooth camera follow
        let target = self.player.pos + self.player.size * 0.5 - self.viewport_size * 0.5;
        let max_cam = (Vec2::new(self.map.width as f32, self.map.height as f32) * self.map.tile_size - self.viewport_size).max(Vec2::ZERO);
        let clamped = target.clamp(Vec2::ZERO, max_cam);
        let lerp_factor = (dt * 8.5).min(1.0);
        self.camera_pos = self.camera_pos.lerp(clamped, lerp_factor);
    }

    //------------[Render game frame]-------------------
    pub fn render(&self, renderer: &mut WgpuRenderer) {
        renderer.set_camera(self.camera_pos, self.viewport_size);

        // Background tile layer
        let bg_tile_size = 64.0;
        let uv_max_x = self.viewport_size.x / bg_tile_size;
        let uv_max_y = self.viewport_size.y / bg_tile_size;

        renderer.draw_texture_rect(
            self.camera_pos,
            self.viewport_size,
            Vec2::ZERO,
            Vec2::new(uv_max_x, uv_max_y),
            self.bg_texture.id,
            &self.bg_texture.bind_group,
            [0.85, 0.85, 0.95, 1.0],
        );

        // Map geometry (ts_main hidden unless debug is on)
        self.map.render(renderer, self.camera_pos, self.viewport_size, self.debug_mode);

        // Player sprite
        self.player.render(renderer);

        // Debug overlays
        if self.debug_mode {
            self.render_debug_overlay(renderer);
        }

        // Restart fade overlay
        if self.fade_alpha > 0.005 {
            renderer.draw_quad(
                self.camera_pos,
                self.viewport_size,
                [0.0, 0.0, 0.0, self.fade_alpha],
            );
        }
    }

    //------------[Render debug overlays and diagnostic HUD]-------------------
    fn render_debug_overlay(&self, renderer: &mut WgpuRenderer) {
        let view_box = crate::physics::AABB::from_pos_size(
            self.camera_pos - Vec2::splat(64.0),
            self.viewport_size + Vec2::splat(128.0),
        );

        // Solid colliders (blue)
        for solid in &self.map.solid_colliders {
            if view_box.overlaps(solid) {
                renderer.draw_debug_box(
                    solid.min,
                    solid.max - solid.min,
                    [0.10, 0.40, 0.85, 0.18],
                    [0.20, 0.60, 1.00, 0.65],
                );
            }
        }

        // One-way platforms (yellow)
        for plat in &self.map.one_way_platforms {
            if view_box.overlaps(plat) {
                renderer.draw_debug_box(
                    plat.min,
                    plat.max - plat.min,
                    [0.90, 0.70, 0.05, 0.22],
                    [1.00, 0.85, 0.10, 0.85],
                );
            }
        }

        // Hazard colliders (red)
        for hazard in &self.map.hazard_colliders {
            if view_box.overlaps(hazard) {
                renderer.draw_debug_box(
                    hazard.min,
                    hazard.max - hazard.min,
                    [0.95, 0.05, 0.05, 0.35],
                    [1.00, 0.15, 0.15, 0.95],
                );
            }
        }

        // Player physics box (green)
        let p_box = self.player.aabb();
        renderer.draw_debug_box(
            p_box.min,
            p_box.max - p_box.min,
            [0.10, 0.90, 0.20, 0.28],
            [0.20, 1.00, 0.30, 0.95],
        );

        // Player hurtbox (orange)
        let h_box = self.player.hurtbox();
        renderer.draw_debug_box(
            h_box.min,
            h_box.max - h_box.min,
            [0.95, 0.35, 0.05, 0.20],
            [1.00, 0.45, 0.10, 0.90],
        );

        // Wall probes
        let left_probe = Vec2::new(self.player.pos.x - 2.0, self.player.pos.y + 4.0);
        let right_probe = Vec2::new(self.player.pos.x + self.player.size.x, self.player.pos.y + 4.0);
        let probe_size = Vec2::new(2.0, self.player.size.y - 8.0);
        let l_col = if self.player.walled_left { [1.0, 0.85, 0.0, 0.95] } else { [0.3, 0.3, 0.3, 0.4] };
        let r_col = if self.player.walled_right { [1.0, 0.85, 0.0, 0.95] } else { [0.3, 0.3, 0.3, 0.4] };
        renderer.draw_quad(left_probe, probe_size, l_col);
        renderer.draw_quad(right_probe, probe_size, r_col);

        // Diagnostic HUD panel
        let panel_pos = self.camera_pos + Vec2::new(6.0, 6.0);
        let panel_size = Vec2::new(315.0, 158.0);
        renderer.draw_debug_box(
            panel_pos,
            panel_size,
            [0.02, 0.03, 0.05, 0.85],
            [0.20, 0.55, 0.90, 0.80],
        );

        let speed = self.player.vel.length();
        let dash_status = if self.player.can_dash { "READY" } else { "BUSY" };
        let step_label = if self.step_hz > 0.0 {
            format!("{:.0} Hz", self.step_hz)
        } else {
            "Dynamic".to_string()
        };

        let hud_lines = format!(
            "[DEBUG MONITOR - F3]\nFPS: {} ({:.1} ms) | STEP: {}\nBACKEND: {} | TIMESTEP: {}\nPOS: X: {:.1}, Y: {:.1}\nVEL: X: {:.1}, Y: {:.1} (SPD: {:.1})\nSTATE: {:?} | GRND: {} | FACE: {:.0}\nWALL: L:{} R:{} | SLIDE: {:.2}s\nDASH: {} | CD: {:.2}s\nMODE: {}\n[F4] CYCLE WINDOW MODE",
            self.fps,
            self.frame_time_ms,
            step_label,
            self.active_backend_name,
            self.timestep_mode_name,
            self.player.pos.x,
            self.player.pos.y,
            self.player.vel.x,
            self.player.vel.y,
            speed,
            self.player.state,
            self.player.grounded,
            self.player.facing,
            self.player.walled_left,
            self.player.walled_right,
            self.player.wall_slide_timer,
            dash_status,
            self.player.dash_cooldown,
            self.current_window_mode,
        );

        renderer.draw_debug_text(
            &hud_lines,
            panel_pos + Vec2::new(6.0, 6.0),
            [0.90, 0.95, 1.00, 1.0],
            0.80,
        );
    }
}
