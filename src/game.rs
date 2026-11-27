//! Gameplay coordinator managing map state, player entity, camera tracking, and HUD.

use glam::Vec2;
use crate::engine::{InputState, Key, SfxEvent, Texture, WgpuRenderer};
use crate::map::TmxMap;
use crate::physics::PushableBox;
use crate::player::Player;

pub struct Game {
    pub map: TmxMap,
    pub player: Player,
    pub crates: Vec<PushableBox>,
    pub bg_texture: Texture,
    pub camera_pos: Vec2,
    pub prev_camera_pos: Vec2,
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
    pub active_note: Option<usize>,
    pub nearby_note: Option<usize>,
    pub note_paper_texture: Option<Texture>,
}

impl Game {
    //------------[Load optional background texture for note panel if present]-------------------
    fn load_optional_note_texture(renderer: &WgpuRenderer) -> Option<Texture> {
        Texture::from_file(
            &renderer.device,
            &renderer.queue,
            "assets/ui/note_paper.png",
            &renderer.texture_bind_group_layout,
        )
        .ok()
    }

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
        let note_paper_texture = Self::load_optional_note_texture(renderer);

        // 640x360 virtual viewport (2x scale on 1280x720)
        let viewport_size = Vec2::new(640.0, 360.0);
        let crates = map.crate_spawns.iter().map(|&pos| PushableBox::new(pos)).collect();
        let mut game = Self {
            map,
            player,
            crates,
            bg_texture,
            camera_pos: Vec2::ZERO,
            prev_camera_pos: Vec2::ZERO,
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
            active_note: None,
            nearby_note: None,
            note_paper_texture,
        };
        game.snap_camera_to_player();
        Ok(game)
    }

    //------------[Check whether a note panel is currently open]-------------------
    pub fn is_note_open(&self) -> bool {
        self.active_note.is_some()
    }

    //------------[Find closest note within interaction range of player]-------------------
    fn find_nearby_note(&self) -> Option<usize> {
        let player_center = self.player.pos + self.player.size * 0.5;
        let mut best: Option<(usize, f32)> = None;
        for (idx, note) in self.map.notes.iter().enumerate() {
            let note_center = note.pos + note.size * 0.5;
            let dist = player_center.distance(note_center);
            if dist <= 36.0 && best.is_none_or(|(_, best_dist)| dist < best_dist) {
                best = Some((idx, dist));
            }
        }
        best.map(|(idx, _)| idx)
    }

    //------------[Sync previous positions to current ones while frozen on note]-------------------
    fn freeze_previous_positions(&mut self) {
        self.prev_camera_pos = self.camera_pos;
        self.player.prev_pos = self.player.pos;
        for krate in &mut self.crates {
            krate.prev_pos = krate.pos;
        }
    }

    //------------[Center camera on player]-------------------
    pub fn snap_camera_to_player(&mut self) {
        let target = self.player.pos + self.player.size * 0.5 - self.viewport_size * 0.5;
        let max_cam = (Vec2::new(self.map.width as f32, self.map.height as f32) * self.map.tile_size - self.viewport_size).max(Vec2::ZERO);
        self.camera_pos = target.clamp(Vec2::ZERO, max_cam);
        self.prev_camera_pos = self.camera_pos;
    }

    //------------[Full level restart]-------------------
    pub fn full_restart(&mut self) {
        self.player.reset(self.map.initial_spawn_pos);
        self.player.checkpoint_pos = self.map.initial_spawn_pos;
        for krate in &mut self.crates {
            krate.reset();
        }
        for item in &mut self.map.interactives {
            if item.kind == crate::map::InteractiveKind::Moon {
                item.activated = false;
            }
        }
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
        self.active_note = None;
        self.nearby_note = None;
    }

    //------------[Load map]-------------------
    pub fn change_map(&mut self, map_path: &str, renderer: &WgpuRenderer) -> Result<(), String> {
        self.map = TmxMap::load_from_file(map_path, renderer)?;
        self.note_paper_texture = Self::load_optional_note_texture(renderer);
        self.crates = self
            .map
            .crate_spawns
            .iter()
            .map(|&pos| PushableBox::new(pos))
            .collect();
        self.full_restart();
        Ok(())
    }

    //------------[Respawn at checkpoint]-------------------
    pub fn reset_level(&mut self) {
        let respawn_pos = self.player.checkpoint_pos;
        self.player.reset(respawn_pos);
        for krate in &mut self.crates {
            krate.reset();
        }
        for item in &mut self.map.interactives {
            if item.kind == crate::map::InteractiveKind::Moon {
                item.activated = false;
            }
        }
        self.snap_camera_to_player();
        self.death_counted = false;
        self.active_note = None;
        self.nearby_note = None;
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

        // Pause gameplay completely while reading a note and wait for close key
        if self.active_note.is_some() {
            self.freeze_previous_positions();
            if input.is_just_pressed(Key::Interact)
                || input.is_just_pressed(Key::Confirm)
                || input.is_just_pressed(Key::Escape)
            {
                self.active_note = None;
                self.player.pending_sfx.push(SfxEvent::NoteOpen);
            }
            return;
        }

        // Track nearest readable note and open it before player movement or interactions
        let can_interact = !self.player.dead && self.fade_alpha < 0.95 && !self.level_finished;
        self.nearby_note = if can_interact {
            self.find_nearby_note()
        } else {
            None
        };

        if can_interact && input.is_just_pressed(Key::Interact) {
            if let Some(idx) = self.nearby_note {
                self.active_note = Some(idx);
                self.player.vel = Vec2::ZERO;
                self.freeze_previous_positions();
                self.player.pending_sfx.push(SfxEvent::NoteOpen);
                return;
            }
        }

        // Level timer
        if !self.level_finished {
            self.level_time += dt;
        }

        // Bottom pit death
        let map_bottom = self.map.height as f32 * self.map.tile_size;
        if self.player.pos.y > map_bottom && !self.player.dead {
            self.player.dead = true;
            self.player.pending_sfx.push(SfxEvent::Death);
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
                    if !self.level_finished {
                        self.player.pending_sfx.push(SfxEvent::LevelComplete);
                    }
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

        // Option 1A: 1. Update pushable crates physics first
        let map_bottom = self.map.height as f32 * self.map.tile_size;
        PushableBox::update_all(
            &mut self.crates,
            dt,
            &self.map.solid_colliders,
            &self.map.one_way_platforms,
            &self.map.ice_colliders,
            &self.map.void_colliders,
            &self.map.spike_colliders,
            map_bottom,
        );

        // Simulation tick when not in blackout
        if self.fade_alpha < 0.95 && !self.level_finished {
            self.player.auto_bhop = self.auto_bhop;
            self.player.update(
                dt,
                input,
                &self.map.solid_colliders,
                &self.map.one_way_platforms,
                &self.map.void_colliders,
                &self.map.spike_colliders,
                &self.map.ice_colliders,
                &mut self.crates,
                &mut self.map.interactives,
            );
        }

        // Smooth camera follow
        self.prev_camera_pos = self.camera_pos;
        let target = self.player.pos + self.player.size * 0.5 - self.viewport_size * 0.5;
        let max_cam = (Vec2::new(self.map.width as f32, self.map.height as f32) * self.map.tile_size - self.viewport_size).max(Vec2::ZERO);
        let clamped = target.clamp(Vec2::ZERO, max_cam);
        let lerp_factor = (dt * 8.5).min(1.0);
        self.camera_pos = self.camera_pos.lerp(clamped, lerp_factor);
    }

    //------------[Render game frame]-------------------
    pub fn render(&self, renderer: &mut WgpuRenderer, alpha: f32) {
        self.render_world(renderer, alpha);
        if self.debug_mode {
            renderer.begin_ui_phase();
            self.render_ui(renderer, alpha);
        }
    }

    //------------[Render world layer subject to PSX post-processing]-------------------
    pub fn render_world(&self, renderer: &mut WgpuRenderer, alpha: f32) {
        let interp_camera = self.prev_camera_pos.lerp(self.camera_pos, alpha);
        renderer.set_camera(interp_camera, self.viewport_size);

        // Background tile layer
        let bg_tile_size = 64.0;
        let uv_max_x = self.viewport_size.x / bg_tile_size;
        let uv_max_y = self.viewport_size.y / bg_tile_size;

        renderer.draw_texture_rect(
            interp_camera,
            self.viewport_size,
            Vec2::ZERO,
            Vec2::new(uv_max_x, uv_max_y),
            self.bg_texture.id,
            &self.bg_texture.bind_group,
            [0.85, 0.85, 0.95, 1.0],
        );

        // Map geometry (ts_main hidden unless debug is on)
        self.map.render(renderer, interp_camera, self.viewport_size, self.debug_mode);

        // Pushable crates (brown rectangle with dark outline)
        for krate in &self.crates {
            if krate.active {
                let interp_pos = krate.prev_pos.lerp(krate.pos, alpha);
                renderer.draw_quad(
                    interp_pos,
                    krate.size,
                    [0.55, 0.35, 0.18, 1.0],
                );
                renderer.draw_rect_outline(
                    interp_pos,
                    krate.size,
                    1.5,
                    [0.28, 0.16, 0.07, 1.0],
                );
            }
        }

        // Player sprite
        self.player.render(renderer, alpha);

        // Restart fade overlay rendered in world phase to apply PSX post-processing
        if self.fade_alpha > 0.005 {
            renderer.draw_quad(
                interp_camera,
                self.viewport_size,
                [0.0, 0.0, 0.0, self.fade_alpha],
            );
        }

        // Draw [W] world prompt above the nearest readable note when no note is open
        if self.active_note.is_none() {
            if let Some(idx) = self.nearby_note {
                if let Some(note) = self.map.notes.get(idx) {
                    let label = "[W]";
                    let scale = 0.75;
                    let text_w = label.len() as f32 * 8.0 * scale;
                    let text_h = 8.0 * scale;
                    let pad_x = 4.0;
                    let pad_y = 3.0;
                    let badge_size = Vec2::new(text_w + pad_x * 2.0, text_h + pad_y * 2.0);
                    let badge_pos = Vec2::new(
                        note.pos.x + (note.size.x - badge_size.x) * 0.5,
                        note.pos.y - badge_size.y - 4.0,
                    );
                    renderer.draw_debug_box(
                        badge_pos,
                        badge_size,
                        [0.06, 0.06, 0.10, 0.88],
                        [0.75, 0.75, 0.85, 0.90],
                    );
                    renderer.draw_debug_text(
                        label,
                        badge_pos + Vec2::new(pad_x, pad_y),
                        [0.96, 0.96, 1.0, 1.0],
                        scale,
                    );
                }
            }
        }

        // Draw full-screen darkened panel when reading a note
        if let Some(idx) = self.active_note {
            if let Some(note) = self.map.notes.get(idx) {
                self.render_note_overlay(renderer, note);
            }
        }
    }

    //------------[Render centered note panel overlay with pause-style dimming]-------------------
    fn render_note_overlay(&self, renderer: &mut WgpuRenderer, note: &crate::map::NoteObject) {
        renderer.set_camera(Vec2::ZERO, self.viewport_size);
        // Darken background identically to MenuScreen::Pause
        renderer.draw_quad(Vec2::ZERO, self.viewport_size, [0.0, 0.0, 0.0, 0.65]);

        let panel_w = 420.0;
        let panel_h = 240.0;
        let panel_pos = Vec2::new(
            (self.viewport_size.x - panel_w) * 0.5,
            (self.viewport_size.y - panel_h) * 0.5,
        );

        // Use optional paper texture if present, otherwise draw simple dark box
        let (text_color, title_color, footer_color) = if let Some(tex) = &self.note_paper_texture {
            renderer.draw_sprite(panel_pos, Vec2::new(panel_w, panel_h), tex);
            (
                [0.12, 0.10, 0.08, 1.0],
                [0.24, 0.12, 0.05, 1.0],
                [0.28, 0.22, 0.18, 0.90],
            )
        } else {
            renderer.draw_debug_box(
                panel_pos,
                Vec2::new(panel_w, panel_h),
                [0.07, 0.07, 0.10, 0.94],
                [0.42, 0.42, 0.52, 0.90],
            );
            (
                [0.92, 0.92, 0.96, 1.0],
                [0.95, 0.85, 0.55, 1.0],
                [0.65, 0.65, 0.75, 0.85],
            )
        };

        let pad_x = 24.0;
        let usable_w = panel_w - pad_x * 2.0;

        let title_trimmed = note.title.trim();
        let body_start_y = if !title_trimmed.is_empty() {
            let title_scale = 1.1;
            let max_title_chars = (usable_w / (8.0 * title_scale)).floor().max(1.0) as usize;
            let display_title: String = title_trimmed.chars().take(max_title_chars).collect();
            let title_w = display_title.chars().count() as f32 * 8.0 * title_scale;
            let title_x = panel_pos.x + (panel_w - title_w) * 0.5;
            renderer.draw_debug_text(
                &display_title,
                Vec2::new(title_x, panel_pos.y + 18.0),
                title_color,
                title_scale,
            );
            panel_pos.y + 42.0
        } else {
            panel_pos.y + 22.0
        };

        let footer = "[W / SPACE / ESC] CLOSE";
        let footer_scale = 0.8;
        let footer_w = footer.len() as f32 * 8.0 * footer_scale;
        let footer_y = panel_pos.y + panel_h - 20.0;
        renderer.draw_debug_text(
            footer,
            Vec2::new(panel_pos.x + (panel_w - footer_w) * 0.5, footer_y),
            footer_color,
            footer_scale,
        );

        let body_scale = 0.9;
        let line_step = 13.0;
        let max_chars = (usable_w / (8.0 * body_scale)).floor().max(1.0) as usize;
        let max_lines = ((footer_y - 12.0 - body_start_y) / line_step)
            .floor()
            .max(1.0) as usize;

        let wrapped = crate::map::wrap_note_text(&note.text, max_chars);
        for (i, line) in wrapped.into_iter().take(max_lines).enumerate() {
            if line.is_empty() {
                continue;
            }
            let line_y = body_start_y + i as f32 * line_step;
            renderer.draw_debug_text(
                &line,
                Vec2::new(panel_pos.x + pad_x, line_y),
                text_color,
                body_scale,
            );
        }
    }

    //------------[Render UI overlays and diagnostic HUD without PSX post-processing]-------------------
    pub fn render_ui(&self, renderer: &mut WgpuRenderer, alpha: f32) {
        // Exit immediately when debug HUD is off to avoid redundant camera uniform uploads
        if !self.debug_mode {
            return;
        }

        let interp_camera = self.prev_camera_pos.lerp(self.camera_pos, alpha);
        renderer.set_camera(interp_camera, self.viewport_size);
        self.render_debug_overlay(renderer, interp_camera, alpha);
    }

    //------------[Render debug overlays and diagnostic HUD]-------------------
    fn render_debug_overlay(&self, renderer: &mut WgpuRenderer, interp_camera: Vec2, alpha: f32) {
        let view_box = crate::physics::AABB::from_pos_size(
            interp_camera - Vec2::splat(64.0),
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

        // Ice colliders (cyan)
        for ice in &self.map.ice_colliders {
            if view_box.overlaps(ice) {
                renderer.draw_debug_box(
                    ice.min,
                    ice.max - ice.min,
                    [0.15, 0.80, 1.00, 0.25],
                    [0.35, 0.95, 1.00, 0.95],
                );
            }
        }

        // Void colliders (red)
        for void in &self.map.void_colliders {
            if view_box.overlaps(void) {
                renderer.draw_debug_box(
                    void.min,
                    void.max - void.min,
                    [0.95, 0.05, 0.05, 0.35],
                    [1.00, 0.15, 0.15, 0.95],
                );
            }
        }

        // Spike colliders (orange)
        for spike in &self.map.spike_colliders {
            if view_box.overlaps(spike) {
                renderer.draw_debug_box(
                    spike.min,
                    spike.max - spike.min,
                    [0.95, 0.40, 0.05, 0.30],
                    [1.00, 0.55, 0.10, 0.90],
                );
            }
        }

        // Note interaction zones (violet)
        for note in &self.map.notes {
            let n_box = crate::physics::AABB::from_pos_size(note.pos, note.size);
            if view_box.overlaps(&n_box) {
                renderer.draw_debug_box(
                    note.pos,
                    note.size,
                    [0.75, 0.40, 0.95, 0.18],
                    [0.88, 0.58, 1.00, 0.85],
                );
            }
        }

        // Pushable crates (orange-yellow)
        for krate in &self.crates {
            if krate.active {
                let interp_pos = krate.prev_pos.lerp(krate.pos, alpha);
                let c_box = crate::physics::AABB::from_pos_size(interp_pos, krate.size);
                if view_box.overlaps(&c_box) {
                    renderer.draw_debug_box(
                        c_box.min,
                        c_box.max - c_box.min,
                        [0.95, 0.65, 0.10, 0.25],
                        [1.00, 0.75, 0.15, 0.95],
                    );
                }
            }
        }

        // Interpolate player position for debug bounding boxes
        let interp_player_pos = self.player.prev_pos.lerp(self.player.pos, alpha);

        // Player physics box (green)
        let p_box = crate::physics::AABB::from_pos_size(interp_player_pos, self.player.size);
        renderer.draw_debug_box(
            p_box.min,
            p_box.max - p_box.min,
            [0.10, 0.90, 0.20, 0.28],
            [0.20, 1.00, 0.30, 0.95],
        );

        // Player hurtbox (orange)
        let inset = Vec2::new(2.0, 2.0);
        let h_box = crate::physics::AABB::new(
            interp_player_pos + inset,
            interp_player_pos + self.player.size - inset,
        );
        renderer.draw_debug_box(
            h_box.min,
            h_box.max - h_box.min,
            [0.95, 0.35, 0.05, 0.20],
            [1.00, 0.45, 0.10, 0.90],
        );

        // Wall probes
        let left_probe = Vec2::new(interp_player_pos.x - 2.0, interp_player_pos.y + 4.0);
        let right_probe = Vec2::new(interp_player_pos.x + self.player.size.x, interp_player_pos.y + 4.0);
        let probe_size = Vec2::new(2.0, self.player.size.y - 8.0);
        let l_col = if self.player.walled_left { [1.0, 0.85, 0.0, 0.95] } else { [0.3, 0.3, 0.3, 0.4] };
        let r_col = if self.player.walled_right { [1.0, 0.85, 0.0, 0.95] } else { [0.3, 0.3, 0.3, 0.4] };
        renderer.draw_quad(left_probe, probe_size, l_col);
        renderer.draw_quad(right_probe, probe_size, r_col);

        // Diagnostic HUD panel
        let panel_pos = interp_camera + Vec2::new(6.0, 6.0);
        let panel_size = Vec2::new(330.0, 184.0);
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

        let last_wall_str = match self.player.last_wall_dir {
            Some(-1) => "L",
            Some(1) => "R",
            _ => "-",
        };

        let form_str = match self.player.form {
            crate::player::PlayerForm::Witch => "WITCH",
            crate::player::PlayerForm::Beast => "BEAST",
        };
        let iframes_str = if self.player.invuln_timer > 0.0 {
            format!("{:.1}s", self.player.invuln_timer)
        } else {
            "-".to_string()
        };
        let ice_str = if self.player.on_ice { "Y" } else { "N" };
        let active_crates = self.crates.iter().filter(|c| c.active).count();
        let total_crates = self.crates.len();

        let hud_lines = format!(
            "[DEBUG MONITOR - F3]\nFPS: {} ({:.1} ms) | STEP: {}\nBACKEND: {} | TIMESTEP: {}\nFORM: {} | IFRAMES: {}\nBOX: {}/{} ACTIVE\nPOS: X: {:.1}, Y: {:.1}\nVEL: X: {:.1}, Y: {:.1} (SPD: {:.1})\nSTATE: {:?} | GRND: {} (ICE:{}) | FACE: {:.0}\nWALL: L:{} R:{} (LAST:{}) | SLIDE: {:.2}s\nDASH: {} | CD: {:.2}s\nMODE: {}\n[F4] CYCLE WINDOW MODE",
            self.fps,
            self.frame_time_ms,
            step_label,
            self.active_backend_name,
            self.timestep_mode_name,
            form_str,
            iframes_str,
            active_crates,
            total_crates,
            self.player.pos.x,
            self.player.pos.y,
            self.player.vel.x,
            self.player.vel.y,
            speed,
            self.player.state,
            self.player.grounded,
            ice_str,
            self.player.facing,
            self.player.walled_left,
            self.player.walled_right,
            last_wall_str,
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
