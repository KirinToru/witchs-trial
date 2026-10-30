pub mod render;
pub mod types;

#[allow(unused_imports)]
pub use types::{
    FpsLockOption, GameSettings, LevelStats, MenuScreen, SettingItem, TimestepMode,
    WindowDisplayMode,
};

use glam::Vec2;
use crate::engine::{GraphicsBackendOption, InputState, Key};

pub struct MenuSystem {
    pub screen: MenuScreen,
    pub main_selected: usize,
    pub map_selected: usize,
    pub available_maps: Vec<String>,
    pub selected_map_path: Option<String>,
    pub settings_selected: usize,
    pub complete_selected: usize,
    pub pause_selected: usize,
    pub settings_return_screen: MenuScreen,
    pub settings: GameSettings,
    pub initial_backend: GraphicsBackendOption,
    pub request_start_game: bool,
    pub request_restart_level: bool,
    pub request_resume_game: bool,
    pub request_exit: bool,
    pub settings_changed: bool,
    pub level_stats: Option<LevelStats>,
}

impl MenuSystem {
    //------------[Scan available map files]-------------------
    pub fn refresh_maps(&mut self) {
        let mut maps = Vec::new();
        let maps_dir = std::path::Path::new("assets/maps");
        if let Ok(entries) = std::fs::read_dir(maps_dir) {
            for entry in entries.flatten() {
                let path = entry.path();
                if path.is_file() {
                    if let Some(ext) = path.extension() {
                        if ext.to_string_lossy().eq_ignore_ascii_case("tmx") {
                            if let Some(filename) = path.file_name() {
                                maps.push(filename.to_string_lossy().to_string());
                            }
                        }
                    }
                }
            }
        }
        maps.sort();
        if maps.is_empty() && std::path::Path::new("assets/maps/test.tmx").exists() {
            maps.push("test.tmx".to_string());
        }
        self.available_maps = maps;
        let total_items = self.available_maps.len() + 1;
        if self.map_selected >= total_items {
            self.map_selected = 0;
        }
    }

    //------------[Create menu system]-------------------
    pub fn new() -> Self {
        let loaded_settings = GameSettings::load_or_default();
        let initial_backend = loaded_settings.graphics_backend;
        let mut menu = Self {
            screen: MenuScreen::Main,
            main_selected: 0,
            map_selected: 0,
            available_maps: Vec::new(),
            selected_map_path: None,
            settings_selected: 0,
            complete_selected: 0,
            pause_selected: 0,
            settings_return_screen: MenuScreen::Main,
            settings: loaded_settings,
            initial_backend,
            request_start_game: false,
            request_restart_level: false,
            request_resume_game: false,
            request_exit: false,
            settings_changed: false,
            level_stats: None,
        };
        menu.refresh_maps();
        menu
    }

    //------------[Filter visible settings]-------------------
    pub fn visible_settings(&self) -> Vec<SettingItem> {
        let mut items = Vec::new();
        items.push(SettingItem::WindowMode);
        items.push(SettingItem::VSync);

        if !self.settings.vsync {
            items.push(SettingItem::MatchTimestep);
            if !self.settings.match_timestep {
                items.push(SettingItem::FpsLock);
            }
        }

        items.push(SettingItem::TimestepMode);
        items.push(SettingItem::AutoBhop);
        items.push(SettingItem::GraphicsBackend);
        items.push(SettingItem::Back);
        items
    }

    //------------[Cycle next setting value]-------------------
    fn activate_setting_next(&mut self, item: SettingItem) {
        match item {
            SettingItem::WindowMode => self.settings.window_mode = self.settings.window_mode.next(),
            SettingItem::VSync => self.settings.vsync = !self.settings.vsync,
            SettingItem::MatchTimestep => self.settings.match_timestep = !self.settings.match_timestep,
            SettingItem::FpsLock => self.settings.fps_lock = self.settings.fps_lock.next(),
            SettingItem::TimestepMode => self.settings.timestep_mode = self.settings.timestep_mode.next(),
            SettingItem::AutoBhop => self.settings.auto_bhop = !self.settings.auto_bhop,
            SettingItem::GraphicsBackend => self.settings.graphics_backend = self.settings.graphics_backend.next(),
            SettingItem::Back => {
                self.screen = self.settings_return_screen;
                return;
            }
        }
        self.settings_changed = true;
        self.settings.save();
    }

    //------------[Cycle previous setting value]-------------------
    fn activate_setting_prev(&mut self, item: SettingItem) {
        match item {
            SettingItem::WindowMode => self.settings.window_mode = self.settings.window_mode.prev(),
            SettingItem::VSync => self.settings.vsync = !self.settings.vsync,
            SettingItem::MatchTimestep => self.settings.match_timestep = !self.settings.match_timestep,
            SettingItem::FpsLock => self.settings.fps_lock = self.settings.fps_lock.prev(),
            SettingItem::TimestepMode => self.settings.timestep_mode = self.settings.timestep_mode.prev(),
            SettingItem::AutoBhop => self.settings.auto_bhop = !self.settings.auto_bhop,
            SettingItem::GraphicsBackend => self.settings.graphics_backend = self.settings.graphics_backend.prev(),
            SettingItem::Back => {
                self.screen = self.settings_return_screen;
                return;
            }
        }
        self.settings_changed = true;
        self.settings.save();
    }

    //------------[Process menu input]-------------------
    pub fn update(&mut self, input: &InputState, _window_size: Vec2, _viewport_size: Vec2) {
        self.request_start_game = false;
        self.request_restart_level = false;
        self.request_resume_game = false;
        self.request_exit = false;
        self.settings_changed = false;

        match self.screen {
            MenuScreen::Main => {
                // Main menu: 0: Start, 1: Settings, 2: Exit
                if input.is_just_pressed(Key::Up) {
                    if self.main_selected == 0 {
                        self.main_selected = 2;
                    } else {
                        self.main_selected -= 1;
                    }
                }
                if input.is_just_pressed(Key::Down) {
                    self.main_selected = (self.main_selected + 1) % 3;
                }

                if input.is_just_pressed(Key::Confirm) {
                    match self.main_selected {
                        0 => {
                            self.refresh_maps();
                            self.screen = MenuScreen::MapSelect;
                            self.map_selected = 0;
                        }
                        1 => {
                            self.settings_return_screen = MenuScreen::Main;
                            self.screen = MenuScreen::Settings;
                            self.settings_selected = 0;
                        }
                        2 => self.request_exit = true,
                        _ => {}
                    }
                }
            }
            MenuScreen::MapSelect => {
                // Map select list navigation
                let total_items = self.available_maps.len() + 1;

                if input.is_just_pressed(Key::Up) {
                    if self.map_selected == 0 {
                        self.map_selected = total_items.saturating_sub(1);
                    } else {
                        self.map_selected -= 1;
                    }
                }
                if input.is_just_pressed(Key::Down) {
                    if total_items > 0 {
                        self.map_selected = (self.map_selected + 1) % total_items;
                    }
                }

                if input.is_just_pressed(Key::Confirm) {
                    if self.map_selected < self.available_maps.len() {
                        let chosen_file = &self.available_maps[self.map_selected];
                        self.selected_map_path = Some(format!("assets/maps/{}", chosen_file));
                        self.request_start_game = true;
                    } else {
                        self.screen = MenuScreen::Main;
                    }
                }

                if input.is_just_pressed(Key::Escape) {
                    self.screen = MenuScreen::Main;
                }
            }
            MenuScreen::Settings => {
                // Settings navigation & adjust
                let visible = self.visible_settings();
                let total_settings = visible.len();
                if self.settings_selected >= total_settings {
                    self.settings_selected = total_settings.saturating_sub(1);
                }

                if input.is_just_pressed(Key::Up) {
                    if self.settings_selected == 0 {
                        self.settings_selected = total_settings - 1;
                    } else {
                        self.settings_selected -= 1;
                    }
                }
                if input.is_just_pressed(Key::Down) {
                    self.settings_selected = (self.settings_selected + 1) % total_settings;
                }

                if input.is_just_pressed(Key::Left) {
                    if let Some(&item) = visible.get(self.settings_selected) {
                        self.activate_setting_prev(item);
                    }
                }

                if input.is_just_pressed(Key::Right) || input.is_just_pressed(Key::Confirm) {
                    if let Some(&item) = visible.get(self.settings_selected) {
                        self.activate_setting_next(item);
                    }
                }

                if input.is_just_pressed(Key::Escape) {
                    self.screen = self.settings_return_screen;
                }
            }
            MenuScreen::Pause => {
                // Pause menu: 0: Continue, 1: Settings, 2: Main Menu
                if input.is_just_pressed(Key::Up) {
                    if self.pause_selected == 0 {
                        self.pause_selected = 2;
                    } else {
                        self.pause_selected -= 1;
                    }
                }
                if input.is_just_pressed(Key::Down) {
                    self.pause_selected = (self.pause_selected + 1) % 3;
                }

                if input.is_just_pressed(Key::Confirm) {
                    match self.pause_selected {
                        0 => {
                            self.request_resume_game = true;
                        }
                        1 => {
                            self.settings_return_screen = MenuScreen::Pause;
                            self.screen = MenuScreen::Settings;
                            self.settings_selected = 0;
                        }
                        2 => {
                            self.screen = MenuScreen::Main;
                            self.main_selected = 0;
                        }
                        _ => {}
                    }
                }

                if input.is_just_pressed(Key::Escape) {
                    self.request_resume_game = true;
                }
            }
            MenuScreen::LevelComplete => {
                // Level complete: 0: Restart, 1: Map Select, 2: Main Menu
                if input.is_just_pressed(Key::Up) {
                    if self.complete_selected == 0 {
                        self.complete_selected = 2;
                    } else {
                        self.complete_selected -= 1;
                    }
                }
                if input.is_just_pressed(Key::Down) {
                    self.complete_selected = (self.complete_selected + 1) % 3;
                }

                if input.is_just_pressed(Key::Confirm) {
                    match self.complete_selected {
                        0 => self.request_restart_level = true,
                        1 => {
                            self.refresh_maps();
                            self.screen = MenuScreen::MapSelect;
                            self.map_selected = 0;
                        }
                        2 => self.screen = MenuScreen::Main,
                        _ => {}
                    }
                }

                if input.is_just_pressed(Key::Escape) {
                    self.screen = MenuScreen::Main;
                }
            }
        }
    }
}

#[cfg(test)]
mod tests;
