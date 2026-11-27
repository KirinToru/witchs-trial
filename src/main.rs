//! # Witch's Trial Engine and API
//!
//! 2D precision platformer engine built with `wgpu` and `winit`.
//!
//! ## Subsystems
//!
//! - [`engine`]: 2D hardware renderer, input state bitmask, timers, textures, and debug fonts.
//! - [`game`]: Orchestrator managing map state, player entity, camera tracking, and HUD.
//! - [`player`]: Kinematic controller, FSM states, sub-stepped AABB collision resolution, and animation.
//! - [`map`]: Tiled TMX XML parser, dynamic tilesets, UV coordinates, and merged colliders.
//! - [`menu`]: Keyboard-driven UI screens, settings persistence, and pause overlay.
//! - [`physics`]: Axis-Aligned Bounding Box (AABB) intersection and proximity detection.

#![allow(
    clippy::too_many_arguments,
    clippy::collapsible_if,
    clippy::manual_range_contains
)]

mod engine;
mod game;
mod map;
mod menu;
mod physics;
mod player;

use std::sync::Arc;
use std::sync::atomic::{AtomicU32, Ordering};
use std::time::Instant;
use winit::application::ApplicationHandler;
use winit::event::WindowEvent;
use winit::event_loop::{ActiveEventLoop, ControlFlow, EventLoop};
use winit::window::{Fullscreen, Window, WindowId};

static LAST_WINDOW_WIDTH: AtomicU32 = AtomicU32::new(1280);
static LAST_WINDOW_HEIGHT: AtomicU32 = AtomicU32::new(720);

use crate::engine::{
    AudioSystem, FrameLimiter, GraphicsBackendOption, InputState, Key, MusicTrack, TimeAccumulator,
    WgpuRenderer,
};
use crate::game::Game;
use crate::menu::{MenuSystem, WindowDisplayMode};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AppState {
    Menu,
    Playing,
}

struct App {
    renderer: Option<WgpuRenderer>,
    game: Option<Game>,
    menu: MenuSystem,
    audio: AudioSystem,
    state: AppState,
    input: InputState,
    time: TimeAccumulator,
    frame_limiter: FrameLimiter,
    last_frame_instant: Instant,
    current_backend: GraphicsBackendOption,
    last_windowed_size: winit::dpi::PhysicalSize<u32>,
}

impl App {
    //------------[Parse CLI backend flag]-------------------
    fn parse_cli_backend() -> Option<GraphicsBackendOption> {
        let args: Vec<String> = std::env::args().collect();
        for i in 0..args.len() {
            if args[i] == "--backend" || args[i] == "-b" {
                if let Some(val) = args.get(i + 1) {
                    return match val.to_lowercase().as_str() {
                        "vulkan" | "vk" => Some(GraphicsBackendOption::Vulkan),
                        "dx12" | "directx" | "directx12" | "d3d12" => {
                            Some(GraphicsBackendOption::DirectX12)
                        }
                        "opengl" | "gl" => Some(GraphicsBackendOption::OpenGL),
                        "metal" => Some(GraphicsBackendOption::Metal),
                        "auto" => Some(GraphicsBackendOption::Auto),
                        _ => None,
                    };
                }
            }
        }
        None
    }

    //------------[Create application instance]-------------------
    pub fn new() -> Self {
        let cli_backend = Self::parse_cli_backend();
        let mut menu = MenuSystem::new();
        if let Some(backend) = cli_backend {
            menu.settings.graphics_backend = backend;
        }
        let initial_backend = menu.settings.graphics_backend;
        let initial_hz = menu.settings.timestep_mode.hz().unwrap_or(60.0);

        // Initialize disk-based audio system with saved volume settings
        let mut audio = AudioSystem::new();
        audio.set_music_volume(menu.settings.music_volume);
        audio.set_sfx_volume(menu.settings.sfx_volume);

        Self {
            renderer: None,
            game: None,
            menu,
            audio,
            state: AppState::Menu,
            input: InputState::new(),
            time: TimeAccumulator::new(initial_hz),
            frame_limiter: FrameLimiter::new(),
            last_frame_instant: Instant::now(),
            current_backend: initial_backend,
            last_windowed_size: winit::dpi::PhysicalSize::new(1280, 720),
        }
    }

    //------------[Apply window display mode]-------------------
    fn apply_window_mode(window: &Window, event_loop: &ActiveEventLoop, mode: WindowDisplayMode) {
        match mode {
            WindowDisplayMode::Windowed => {
                window.set_fullscreen(None);
                let w = LAST_WINDOW_WIDTH.load(Ordering::Relaxed);
                let h = LAST_WINDOW_HEIGHT.load(Ordering::Relaxed);
                let _ = window.request_inner_size(winit::dpi::PhysicalSize::new(w, h));
            }
            WindowDisplayMode::Borderless => {
                window.set_fullscreen(Some(Fullscreen::Borderless(None)));
            }
            WindowDisplayMode::Exclusive => {
                // Fall back to primary or first available monitor if current_monitor is None at startup
                let monitor = window
                    .current_monitor()
                    .or_else(|| event_loop.primary_monitor())
                    .or_else(|| event_loop.available_monitors().next());
                let video_mode = monitor.and_then(|m| m.video_modes().next());
                if let Some(m) = video_mode {
                    window.set_fullscreen(Some(Fullscreen::Exclusive(m)));
                } else {
                    window.set_fullscreen(Some(Fullscreen::Borderless(None)));
                }
            }
        }
    }

    //------------[Synchronize game with menu settings]-------------------
    fn sync_game_settings(&mut self) {
        if let Some(game) = &mut self.game {
            game.current_window_mode = self.menu.settings.window_mode.label().to_string();
            game.timestep_mode_name = self.menu.settings.timestep_mode.label().to_string();
            game.step_hz = self.menu.settings.timestep_mode.hz().unwrap_or(0.0);
            game.auto_bhop = self.menu.settings.auto_bhop;
        }
    }
}

impl ApplicationHandler for App {
    //------------[Init window and renderer on resume]-------------------
    fn resumed(&mut self, event_loop: &ActiveEventLoop) {
        if self.renderer.is_none() {
            let window_attributes = Window::default_attributes()
                .with_title("Witch's Trial")
                .with_inner_size(winit::dpi::LogicalSize::new(1280.0, 720.0));

            let window = Arc::new(
                event_loop
                    .create_window(window_attributes)
                    .expect("Failed to create window"),
            );

            // Restore saved fullscreen mode right after creating window
            if matches!(
                self.menu.settings.window_mode,
                WindowDisplayMode::Borderless | WindowDisplayMode::Exclusive
            ) {
                Self::apply_window_mode(&window, event_loop, self.menu.settings.window_mode);
            }

            let mut renderer =
                WgpuRenderer::new_with_backend(window, self.menu.settings.graphics_backend);
            renderer.set_vsync(self.menu.settings.vsync);
            renderer.set_psx_shader(self.menu.settings.psx_shader);

            // Sync swapchain and post-process buffers if fullscreen changed window dimensions
            let current_size = renderer.window.inner_size();
            if current_size.width > 0
                && current_size.height > 0
                && (current_size.width != renderer.surface_config.width
                    || current_size.height != renderer.surface_config.height)
            {
                renderer.resize(current_size);
            }

            let default_map = self
                .menu
                .available_maps
                .first()
                .map(|name| format!("assets/maps/{}", name))
                .unwrap_or_else(|| "assets/maps/test.tmx".to_string());

            let mut game = match Game::new(&default_map, &renderer) {
                Ok(g) => g,
                Err(err) => {
                    eprintln!("Error initializing game: {}", err);
                    event_loop.exit();
                    return;
                }
            };

            game.active_backend_name = renderer.active_backend_name.clone();
            self.current_backend = self.menu.settings.graphics_backend;
            self.renderer = Some(renderer);
            self.game = Some(game);
            self.sync_game_settings();
        }
    }

    //------------[Handle window events]-------------------
    fn window_event(
        &mut self,
        event_loop: &ActiveEventLoop,
        _window_id: WindowId,
        event: WindowEvent,
    ) {
        match event {
            WindowEvent::CloseRequested => {
                event_loop.exit();
            }
            WindowEvent::Resized(physical_size) => {
                if let Some(renderer) = &mut self.renderer {
                    // Only save windowed size when in Windowed mode to avoid overwriting with fullscreen size
                    if self.menu.settings.window_mode == WindowDisplayMode::Windowed
                        && renderer.window.fullscreen().is_none()
                        && physical_size.width > 0
                        && physical_size.height > 0
                    {
                        self.last_windowed_size = physical_size;
                        LAST_WINDOW_WIDTH.store(physical_size.width, Ordering::Relaxed);
                        LAST_WINDOW_HEIGHT.store(physical_size.height, Ordering::Relaxed);
                    }
                    renderer.resize(physical_size);
                }
            }
            WindowEvent::CursorMoved { position, .. } => {
                self.input
                    .on_cursor_moved(glam::Vec2::new(position.x as f32, position.y as f32));
            }
            WindowEvent::MouseInput { button, state, .. } => {
                self.input.on_mouse_input(button, state);
            }
            WindowEvent::KeyboardInput {
                event: key_event, ..
            } => {
                self.input
                    .on_key_event(key_event.physical_key, key_event.state);

                // Quick F4 window mode toggle
                if self.input.is_just_pressed(Key::ToggleFullscreen) {
                    self.menu.settings.window_mode = self.menu.settings.window_mode.next();
                    // Save updated window mode immediately so it persists after restart
                    self.menu.settings.save();
                    if let Some(renderer) = &self.renderer {
                        Self::apply_window_mode(
                            &renderer.window,
                            event_loop,
                            self.menu.settings.window_mode,
                        );
                    }
                    if let Some(game) = &mut self.game {
                        game.current_window_mode =
                            self.menu.settings.window_mode.label().to_string();
                    }
                }
            }
            WindowEvent::RedrawRequested => {
                if let (Some(renderer), Some(game)) = (&mut self.renderer, &self.game) {
                    // Skip frame encoding and yield briefly if window is minimized or zero-sized
                    let win_size = renderer.window.inner_size();
                    if win_size.width == 0
                        || win_size.height == 0
                        || renderer.window.is_minimized() == Some(true)
                    {
                        std::thread::sleep(std::time::Duration::from_millis(8));
                        return;
                    }

                    renderer.begin_frame();

                    // In fixed timestep mode use accumulator alpha, in dynamic mode use 1.0
                    let alpha = match self.menu.settings.timestep_mode {
                        crate::menu::TimestepMode::Dynamic => 1.0,
                        _ => self.time.alpha(),
                    };

                    match self.state {
                        AppState::Menu => {
                            let is_paused_view = self.menu.screen == crate::menu::MenuScreen::Pause
                                || (self.menu.screen == crate::menu::MenuScreen::Settings
                                    && self.menu.settings_return_screen == crate::menu::MenuScreen::Pause);

                            if is_paused_view {
                                game.render_world(renderer, alpha);
                            }
                            renderer.set_camera(glam::Vec2::ZERO, game.viewport_size);
                            self.menu.render(renderer, game.viewport_size);

                            if is_paused_view && game.debug_mode {
                                renderer.begin_ui_phase();
                                game.render_ui(renderer, alpha);
                            }
                        }
                        AppState::Playing => {
                            game.render(renderer, alpha);
                        }
                    }

                    // Yield briefly when frame presentation is skipped to avoid spin-looping under ControlFlow::Poll
                    if !renderer.end_frame() {
                        std::thread::sleep(std::time::Duration::from_millis(4));
                    }
                }
            }
            _ => {}
        }
    }

    //------------[Game loop step and frame limiter]-------------------
    fn about_to_wait(&mut self, event_loop: &ActiveEventLoop) {
        let now = Instant::now();
        let frame_duration = now.duration_since(self.last_frame_instant).as_secs_f32();
        self.last_frame_instant = now;

        self.time.update();

        // Track frame time for F3 HUD
        if let Some(game) = &mut self.game {
            game.record_render_frame(frame_duration);
        }

        match self.state {
            AppState::Menu => {
                let (window_size, viewport_size) = match (&self.renderer, &self.game) {
                    (Some(r), Some(g)) => (r.window_size(), g.viewport_size),
                    _ => (
                        glam::Vec2::new(1280.0, 720.0),
                        glam::Vec2::new(640.0, 360.0),
                    ),
                };

                self.menu.update(&self.input, window_size, viewport_size);
                // Play menu navigation and confirmation sound effects
                for sfx in self.menu.pending_sfx.drain(..) {
                    self.audio.play_sfx(sfx);
                }

                if self.menu.request_start_game {
                    if let (Some(game), Some(renderer)) = (&mut self.game, &self.renderer) {
                        if let Some(map_path) = &self.menu.selected_map_path {
                            if let Err(err) = game.change_map(map_path, renderer) {
                                eprintln!("Error loading map {}: {}", map_path, err);
                            }
                        }
                    }
                    self.sync_game_settings();
                    if let Some(game) = &mut self.game {
                        game.full_restart();
                    }
                    self.state = AppState::Playing;
                    // Discard time spent in menu and map loading before first play frame
                    self.time.reset_accumulator();
                }

                if self.menu.request_restart_level {
                    self.sync_game_settings();
                    if let Some(game) = &mut self.game {
                        game.full_restart();
                    }
                    self.state = AppState::Playing;
                    // Discard menu time before restarting level
                    self.time.reset_accumulator();
                }

                if self.menu.request_exit {
                    event_loop.exit();
                    return;
                }

                if self.menu.request_resume_game {
                    self.state = AppState::Playing;
                    // Prevent catching up on time spent in pause menu
                    self.time.reset_accumulator();
                    self.input.clear_frame_edges();
                }

                if self.menu.settings_changed {
                    if let Some(renderer) = &mut self.renderer {
                        Self::apply_window_mode(
                            &renderer.window,
                            event_loop,
                            self.menu.settings.window_mode,
                        );
                        renderer.set_vsync(self.menu.settings.vsync);
                        renderer.set_psx_shader(self.menu.settings.psx_shader);
                    }
                    // Apply updated volume percentages immediately
                    self.audio.set_music_volume(self.menu.settings.music_volume);
                    self.audio.set_sfx_volume(self.menu.settings.sfx_volume);
                    self.sync_game_settings();
                    // Sync target timestep frequency and reset backlog on settings change
                    if let Some(hz) = self.menu.settings.timestep_mode.hz() {
                        self.time.set_hz(hz);
                    }
                    self.time.reset_accumulator();
                }

                // Update music track and pause state according to current screen
                if self.state == AppState::Playing {
                    self.audio.set_music_paused(false);
                    self.audio.play_music(MusicTrack::Level);
                } else {
                    let is_paused_view = self.menu.screen == crate::menu::MenuScreen::Pause
                        || (self.menu.screen == crate::menu::MenuScreen::Settings
                            && self.menu.settings_return_screen == crate::menu::MenuScreen::Pause);
                    if is_paused_view {
                        self.audio.set_music_paused(true);
                    } else {
                        self.audio.set_music_paused(false);
                        self.audio.play_music(MusicTrack::Menu);
                    }
                }

                // Keep accumulator at zero while staying in menu or pause
                if self.state == AppState::Menu {
                    self.time.reset_accumulator();
                }

                self.input.clear_frame_edges();
            }
            AppState::Playing => {
                let note_open = self
                    .game
                    .as_ref()
                    .map(|g| g.is_note_open())
                    .unwrap_or(false);

                // Check level completion
                let level_just_finished = self
                    .game
                    .as_ref()
                    .map(|g| g.level_finished)
                    .unwrap_or(false);
                if level_just_finished {
                    if let Some(game) = &mut self.game {
                        for sfx in game.player.pending_sfx.drain(..) {
                            self.audio.play_sfx(sfx);
                        }
                        self.menu.level_stats = Some(crate::menu::LevelStats {
                            time_secs: game.level_time,
                            deaths: game.deaths_count,
                            jumps: game.player.jumps_count,
                            dashes: game.player.dashes_count,
                        });
                    }
                    self.menu.screen = crate::menu::MenuScreen::LevelComplete;
                    self.state = AppState::Menu;
                    self.audio.set_music_paused(false);
                    self.audio.play_music(MusicTrack::Menu);
                    self.time.reset_accumulator();
                    self.input.clear_frame_edges();
                } else if self.input.is_just_pressed(Key::Escape) && !note_open {
                    // Open pause menu, pause level music, and clear pending physics backlog
                    self.state = AppState::Menu;
                    self.menu.screen = crate::menu::MenuScreen::Pause;
                    self.menu.pause_selected = 0;
                    self.audio.set_music_paused(true);
                    self.time.reset_accumulator();
                    self.input.clear_frame_edges();
                } else {
                    self.audio.set_music_paused(false);
                    self.audio.play_music(MusicTrack::Level);

                    let current_hz = self.menu.settings.timestep_mode.hz();
                    let current_label = self.menu.settings.timestep_mode.label().to_string();
                    if let Some(game) = &mut self.game {
                        game.timestep_mode_name = current_label;
                        game.step_hz = current_hz.unwrap_or(0.0);
                    }

                    if note_open {
                        // Freeze fixed-timestep backlog and process note closing regardless of accumulator
                        self.time.reset_accumulator();
                        if let Some(game) = &mut self.game {
                            game.update(0.0, &self.input);
                            for sfx in game.player.pending_sfx.drain(..) {
                                self.audio.play_sfx(sfx);
                            }
                        }
                        self.input.clear_frame_edges();
                    } else {
                        match self.menu.settings.timestep_mode {
                            crate::menu::TimestepMode::Dynamic => {
                                // Clear fixed-timestep accumulator so Dynamic mode never builds backlog
                                self.time.reset_accumulator();
                                // Variable timestep step
                                let dt = frame_duration.min(0.05);
                                if let Some(game) = &mut self.game {
                                    game.update(dt, &self.input);
                                    for sfx in game.player.pending_sfx.drain(..) {
                                        self.audio.play_sfx(sfx);
                                    }
                                }
                                self.input.clear_frame_edges();
                            }
                            mode => {
                                // Fixed timestep loop
                                if let Some(hz) = mode.hz() {
                                    self.time.set_hz(hz);
                                }
                                let dt = self.time.dt();
                                while self.time.step() {
                                    if let Some(game) = &mut self.game {
                                        game.update(dt, &self.input);
                                        for sfx in game.player.pending_sfx.drain(..) {
                                            self.audio.play_sfx(sfx);
                                        }
                                        // Stop extra fixed steps right away if opening a note paused simulation
                                        if game.is_note_open() {
                                            self.time.reset_accumulator();
                                            self.input.clear_frame_edges();
                                            break;
                                        }
                                    }
                                    self.input.clear_frame_edges();
                                }
                            }
                        }
                    }
                }
            }
        }

        // Frame rate limiter (lock FPS or match timestep)
        let target_fps = if self.menu.settings.vsync {
            None
        } else if self.menu.settings.match_timestep {
            self.menu.settings.timestep_mode.hz().map(|hz| hz as u32)
        } else {
            self.menu.settings.fps_lock.target_fps()
        };

        self.frame_limiter.set_target_fps(target_fps);
        self.frame_limiter.wait_remaining(now);

        if let Some(renderer) = &self.renderer {
            renderer.window.request_redraw();
        }
    }
}

//------------[Entry point]-------------------
fn main() -> Result<(), Box<dyn std::error::Error>> {
    env_logger::init();
    let event_loop = EventLoop::new()?;
    event_loop.set_control_flow(ControlFlow::Poll);

    let mut app = App::new();
    event_loop.run_app(&mut app)?;
    Ok(())
}
