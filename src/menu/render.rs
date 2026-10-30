use glam::Vec2;
use crate::engine::WgpuRenderer;
use super::{LevelStats, MenuScreen, MenuSystem, SettingItem};

impl MenuSystem {
    //------------[Render active menu screen]-------------------
    pub fn render(&self, renderer: &mut WgpuRenderer, viewport_size: Vec2) {
        // Darken background: translucent for pause, solid black for main menus
        match self.screen {
            MenuScreen::Pause => {
                renderer.draw_quad(Vec2::ZERO, viewport_size, [0.0, 0.0, 0.0, 0.65]);
            }
            MenuScreen::Settings if self.settings_return_screen == MenuScreen::Pause => {
                renderer.draw_quad(Vec2::ZERO, viewport_size, [0.0, 0.0, 0.0, 0.75]);
            }
            _ => {
                renderer.draw_quad(Vec2::ZERO, viewport_size, [0.0, 0.0, 0.0, 1.0]);
            }
        }

        match self.screen {
            MenuScreen::Main => {
                // Title
                let title = "Witch's Trial";
                let title_scale = 2.4;
                let title_w = title.len() as f32 * 8.0 * title_scale;
                let title_x = (viewport_size.x - title_w) * 0.5;
                let title_y = 65.0;

                // Title shadow
                renderer.draw_debug_text(
                    title,
                    Vec2::new(title_x + 2.0, title_y + 2.0),
                    [0.25, 0.02, 0.02, 0.9],
                    title_scale,
                );
                // Title foreground
                renderer.draw_debug_text(
                    title,
                    Vec2::new(title_x, title_y),
                    [0.98, 0.12, 0.12, 1.0],
                    title_scale,
                );

                // Options list
                let options = ["START GAME", "SETTINGS", "EXIT"];
                let start_y = 150.0;
                let opt_scale = 1.25;

                for (idx, &opt) in options.iter().enumerate() {
                    let y = start_y + idx as f32 * 36.0;
                    let is_selected = self.main_selected == idx;

                    let label = if is_selected {
                        format!("> {} <", opt)
                    } else {
                        format!("  {}  ", opt)
                    };

                    let w = label.len() as f32 * 8.0 * opt_scale;
                    let x = (viewport_size.x - w) * 0.5;

                    if is_selected {
                        let box_pad_x = 12.0;
                        let box_pad_y = 4.0;
                        renderer.draw_debug_box(
                            Vec2::new(x - box_pad_x, y - box_pad_y),
                            Vec2::new(w + box_pad_x * 2.0, 8.0 * opt_scale + box_pad_y * 2.0),
                            [0.25, 0.04, 0.04, 0.55],
                            [0.95, 0.20, 0.20, 0.90],
                        );
                        renderer.draw_debug_text(
                            &label,
                            Vec2::new(x, y),
                            [1.0, 0.95, 0.95, 1.0],
                            opt_scale,
                        );
                    } else {
                        renderer.draw_debug_text(
                            &label,
                            Vec2::new(x, y),
                            [0.70, 0.70, 0.78, 0.85],
                            opt_scale,
                        );
                    }
                }

                // Footer Controls Prompt
                let footer = "[W / S] Navigate  |  [ENTER / SPACE] Select";
                let foot_scale = 0.8;
                let foot_w = footer.len() as f32 * 8.0 * foot_scale;
                let foot_x = (viewport_size.x - foot_w) * 0.5;
                renderer.draw_debug_text(
                    footer,
                    Vec2::new(foot_x, viewport_size.y - 30.0),
                    [0.45, 0.45, 0.55, 0.70],
                    foot_scale,
                );
            }
            MenuScreen::MapSelect => {
                // Map Select Header
                let title = "SELECT MAP";
                let title_scale = 1.8;
                let title_w = title.len() as f32 * 8.0 * title_scale;
                let title_x = (viewport_size.x - title_w) * 0.5;
                let title_y = 35.0;

                renderer.draw_debug_text(
                    title,
                    Vec2::new(title_x + 2.0, title_y + 2.0),
                    [0.25, 0.02, 0.02, 0.9],
                    title_scale,
                );
                renderer.draw_debug_text(
                    title,
                    Vec2::new(title_x, title_y),
                    [0.98, 0.16, 0.16, 1.0],
                    title_scale,
                );

                // Container Panel Box
                let panel_w = 460.0;
                let total_items = self.available_maps.len() + 1;
                let row_spacing = 30.0;
                let panel_h = (total_items as f32 * row_spacing + 36.0).clamp(160.0, 260.0);
                let panel_x = (viewport_size.x - panel_w) * 0.5;
                let panel_y = 75.0;

                renderer.draw_debug_box(
                    Vec2::new(panel_x, panel_y),
                    Vec2::new(panel_w, panel_h),
                    [0.05, 0.06, 0.09, 0.88],
                    [0.30, 0.45, 0.65, 0.75],
                );

                let opt_scale = 0.95;
                let row_start_y = panel_y + 18.0;

                if self.available_maps.is_empty() {
                    let empty_msg = "< No maps found in assets/maps >";
                    let w = empty_msg.len() as f32 * 8.0 * opt_scale;
                    let x = (viewport_size.x - w) * 0.5;
                    renderer.draw_debug_text(
                        empty_msg,
                        Vec2::new(x, row_start_y),
                        [0.85, 0.45, 0.45, 0.9],
                        opt_scale,
                    );
                } else {
                    for (idx, map_name) in self.available_maps.iter().enumerate() {
                        let y = row_start_y + idx as f32 * row_spacing;
                        let is_selected = self.map_selected == idx;
                        let clean_name = map_name.strip_suffix(".tmx").unwrap_or(map_name.as_str());
                        let prefix = if is_selected { "> " } else { "  " };
                        let full_line = format!("{}{}", prefix, clean_name);

                        if is_selected {
                            renderer.draw_debug_box(
                                Vec2::new(panel_x + 8.0, y - 4.0),
                                Vec2::new(panel_w - 16.0, 8.0 * opt_scale + 8.0),
                                [0.22, 0.05, 0.05, 0.60],
                                [0.90, 0.22, 0.22, 0.85],
                            );
                            renderer.draw_debug_text(
                                &full_line,
                                Vec2::new(panel_x + 16.0, y),
                                [1.0, 0.95, 0.90, 1.0],
                                opt_scale,
                            );
                        } else {
                            renderer.draw_debug_text(
                                &full_line,
                                Vec2::new(panel_x + 16.0, y),
                                [0.72, 0.72, 0.80, 0.85],
                                opt_scale,
                            );
                        }
                    }
                }

                // Back option at the bottom of the list
                let back_idx = self.available_maps.len();
                let back_y = row_start_y + back_idx as f32 * row_spacing;
                let is_back_selected = self.map_selected == back_idx;
                let back_prefix = if is_back_selected { "> " } else { "  " };
                let back_line = format!("{}< Back to Main Menu >", back_prefix);

                if is_back_selected {
                    renderer.draw_debug_box(
                        Vec2::new(panel_x + 8.0, back_y - 4.0),
                        Vec2::new(panel_w - 16.0, 8.0 * opt_scale + 8.0),
                        [0.22, 0.05, 0.05, 0.60],
                        [0.90, 0.22, 0.22, 0.85],
                    );
                    renderer.draw_debug_text(
                        &back_line,
                        Vec2::new(panel_x + 16.0, back_y),
                        [1.0, 0.95, 0.90, 1.0],
                        opt_scale,
                    );
                } else {
                    renderer.draw_debug_text(
                        &back_line,
                        Vec2::new(panel_x + 16.0, back_y),
                        [0.65, 0.65, 0.72, 0.80],
                        opt_scale,
                    );
                }

                let footer = "[W / S] Select  |  [ENTER] Launch  |  [ESC] Back";
                let foot_scale = 0.8;
                let foot_w = footer.len() as f32 * 8.0 * foot_scale;
                let foot_x = (viewport_size.x - foot_w) * 0.5;
                renderer.draw_debug_text(
                    footer,
                    Vec2::new(foot_x, viewport_size.y - 25.0),
                    [0.45, 0.45, 0.55, 0.70],
                    foot_scale,
                );
            }
            MenuScreen::Settings => {
                // Settings Header
                let title = "SETTINGS";
                let title_scale = 1.8;
                let title_w = title.len() as f32 * 8.0 * title_scale;
                let title_x = (viewport_size.x - title_w) * 0.5;
                let title_y = 30.0;

                renderer.draw_debug_text(
                    title,
                    Vec2::new(title_x, title_y),
                    [0.95, 0.18, 0.18, 1.0],
                    title_scale,
                );

                // Settings Container Box
                let panel_w = 460.0;
                let panel_h = 270.0;
                let panel_x = (viewport_size.x - panel_w) * 0.5;
                let panel_y = 65.0;

                renderer.draw_debug_box(
                    Vec2::new(panel_x, panel_y),
                    Vec2::new(panel_w, panel_h),
                    [0.05, 0.06, 0.09, 0.88],
                    [0.30, 0.45, 0.65, 0.75],
                );

                let visible = self.visible_settings();
                let opt_scale = 0.90;
                let row_start_y = panel_y + 16.0;
                let row_spacing = 26.0;

                for (idx, &item) in visible.iter().enumerate() {
                    let y = row_start_y + idx as f32 * row_spacing;
                    let is_selected = self.settings_selected == idx;

                    let text = match item {
                        SettingItem::WindowMode => format!("Window Mode:        < {} >", self.settings.window_mode.label()),
                        SettingItem::VSync => format!("Vsync:              < {} >", if self.settings.vsync { "Enabled" } else { "Disabled" }),
                        SettingItem::MatchTimestep => format!("Match Timestep:     < {} >", if self.settings.match_timestep { "Enabled" } else { "Disabled" }),
                        SettingItem::FpsLock => format!("Lock FPS:           < {} >", self.settings.fps_lock.label()),
                        SettingItem::TimestepMode => format!("Timestep Mode:      < {} >", self.settings.timestep_mode.label()),
                        SettingItem::AutoBhop => format!("Auto Bunny Hop:     < {} >", if self.settings.auto_bhop { "Enabled" } else { "Disabled" }),
                        SettingItem::GraphicsBackend => format!("Graphics Backend:   < {} >", self.settings.graphics_backend.label()),
                        SettingItem::Back => {
                            if self.settings_return_screen == MenuScreen::Pause {
                                "            < Back to Pause Menu >".to_string()
                            } else {
                                "            < Back to Main Menu >".to_string()
                            }
                        }
                    };

                    let prefix = if is_selected { "> " } else { "  " };
                    let full_line = format!("{}{}", prefix, text);

                    if is_selected {
                        renderer.draw_debug_box(
                            Vec2::new(panel_x + 8.0, y - 4.0),
                            Vec2::new(panel_w - 16.0, 8.0 * opt_scale + 8.0),
                            [0.22, 0.05, 0.05, 0.60],
                            [0.90, 0.22, 0.22, 0.85],
                        );
                        renderer.draw_debug_text(
                            &full_line,
                            Vec2::new(panel_x + 16.0, y),
                            [1.0, 0.95, 0.90, 1.0],
                            opt_scale,
                        );
                    } else {
                        renderer.draw_debug_text(
                            &full_line,
                            Vec2::new(panel_x + 16.0, y),
                            [0.72, 0.72, 0.80, 0.85],
                            opt_scale,
                        );
                    }
                }

                // Yellow restart notice shown ONLY when a restart-requiring setting was changed
                if self.settings.graphics_backend != self.initial_backend {
                    let restart_notice = "* Some settings will take effect after game restart.";
                    let notice_scale = 0.75;
                    let notice_w = restart_notice.len() as f32 * 8.0 * notice_scale;
                    let notice_x = (viewport_size.x - notice_w) * 0.5;
                    renderer.draw_debug_text(
                        restart_notice,
                        Vec2::new(notice_x, panel_y + panel_h - 18.0),
                        [0.95, 0.85, 0.20, 0.95],
                        notice_scale,
                    );
                }

                let footer = "[W / S] Select  |  [A / D / ENTER] Adjust  |  [ESC] Back";
                let foot_scale = 0.8;
                let foot_w = footer.len() as f32 * 8.0 * foot_scale;
                let foot_x = (viewport_size.x - foot_w) * 0.5;
                renderer.draw_debug_text(
                    footer,
                    Vec2::new(foot_x, viewport_size.y - 25.0),
                    [0.45, 0.45, 0.55, 0.70],
                    foot_scale,
                );
            }
            MenuScreen::LevelComplete => {
                // Header
                let title = "LEVEL COMPLETE!";
                let title_scale = 2.0;
                let title_w = title.len() as f32 * 8.0 * title_scale;
                let title_x = (viewport_size.x - title_w) * 0.5;
                let title_y = 25.0;

                renderer.draw_debug_text(
                    title,
                    Vec2::new(title_x + 2.0, title_y + 2.0),
                    [0.25, 0.18, 0.02, 0.9],
                    title_scale,
                );
                renderer.draw_debug_text(
                    title,
                    Vec2::new(title_x, title_y),
                    [1.00, 0.85, 0.15, 1.0],
                    title_scale,
                );

                // Statistics Panel Box
                let panel_w = 380.0;
                let panel_h = 110.0;
                let panel_x = (viewport_size.x - panel_w) * 0.5;
                let panel_y = 65.0;

                renderer.draw_debug_box(
                    Vec2::new(panel_x, panel_y),
                    Vec2::new(panel_w, panel_h),
                    [0.05, 0.06, 0.09, 0.92],
                    [0.85, 0.70, 0.15, 0.80],
                );

                let stats = self.level_stats.unwrap_or(LevelStats {
                    time_secs: 0.0,
                    deaths: 0,
                    jumps: 0,
                    dashes: 0,
                });

                let minutes = (stats.time_secs / 60.0).floor() as u32;
                let seconds = (stats.time_secs % 60.0).floor() as u32;
                let millis = ((stats.time_secs - stats.time_secs.floor()) * 100.0).floor() as u32;
                let time_str = format!("{:02}:{:02}.{:02}", minutes, seconds, millis);

                let stat_scale = 0.95;
                let stats_lines = [
                    format!("CLEAR TIME:    {}", time_str),
                    format!("TOTAL DEATHS:  {}", stats.deaths),
                    format!("TOTAL JUMPS:   {}", stats.jumps),
                    format!("TOTAL DASHES:  {}", stats.dashes),
                ];

                for (idx, line) in stats_lines.iter().enumerate() {
                    let y = panel_y + 12.0 + idx as f32 * 22.0;
                    renderer.draw_debug_text(
                        line,
                        Vec2::new(panel_x + 24.0, y),
                        [0.92, 0.95, 1.0, 1.0],
                        stat_scale,
                    );
                }

                // Options: 0: Restart Level, 1: Select Map, 2: Main Menu
                let options = ["RESTART LEVEL", "SELECT MAP", "MAIN MENU"];
                let btn_start_y = 195.0;
                let opt_scale = 1.10;

                for (idx, &opt) in options.iter().enumerate() {
                    let y = btn_start_y + idx as f32 * 34.0;
                    let is_selected = self.complete_selected == idx;

                    let label = if is_selected {
                        format!("> {} <", opt)
                    } else {
                        format!("  {}  ", opt)
                    };

                    let w = label.len() as f32 * 8.0 * opt_scale;
                    let x = (viewport_size.x - w) * 0.5;

                    if is_selected {
                        let pad_x = 12.0;
                        let pad_y = 4.0;
                        renderer.draw_debug_box(
                            Vec2::new(x - pad_x, y - pad_y),
                            Vec2::new(w + pad_x * 2.0, 8.0 * opt_scale + pad_y * 2.0),
                            [0.25, 0.18, 0.04, 0.65],
                            [1.00, 0.85, 0.20, 0.90],
                        );
                        renderer.draw_debug_text(
                            &label,
                            Vec2::new(x, y),
                            [1.0, 0.95, 0.85, 1.0],
                            opt_scale,
                        );
                    } else {
                        renderer.draw_debug_text(
                            &label,
                            Vec2::new(x, y),
                            [0.72, 0.72, 0.80, 0.85],
                            opt_scale,
                        );
                    }
                }

                let footer = "[W / S] Navigate  |  [ENTER / SPACE] Select Option";
                let foot_scale = 0.8;
                let foot_w = footer.len() as f32 * 8.0 * foot_scale;
                let foot_x = (viewport_size.x - foot_w) * 0.5;
                renderer.draw_debug_text(
                    footer,
                    Vec2::new(foot_x, viewport_size.y - 25.0),
                    [0.45, 0.45, 0.55, 0.70],
                    foot_scale,
                );
            }
            MenuScreen::Pause => {
                // Pause Header in Bold Red
                let title = "PAUSED";
                let title_scale = 2.2;
                let title_w = title.len() as f32 * 8.0 * title_scale;
                let title_x = (viewport_size.x - title_w) * 0.5;
                let title_y = 65.0;

                // Subtle shadow
                renderer.draw_debug_text(
                    title,
                    Vec2::new(title_x + 2.0, title_y + 2.0),
                    [0.25, 0.02, 0.02, 0.9],
                    title_scale,
                );
                // Glowing Title
                renderer.draw_debug_text(
                    title,
                    Vec2::new(title_x, title_y),
                    [0.98, 0.16, 0.16, 1.0],
                    title_scale,
                );

                // Pause Options Box: CONTINUE, SETTINGS, MAIN MENU
                let options = ["CONTINUE", "SETTINGS", "MAIN MENU"];
                let start_y = 150.0;
                let opt_scale = 1.25;

                for (idx, &opt) in options.iter().enumerate() {
                    let y = start_y + idx as f32 * 36.0;
                    let is_selected = self.pause_selected == idx;

                    let label = if is_selected {
                        format!("> {} <", opt)
                    } else {
                        format!("  {}  ", opt)
                    };

                    let w = label.len() as f32 * 8.0 * opt_scale;
                    let x = (viewport_size.x - w) * 0.5;

                    if is_selected {
                        let box_pad_x = 12.0;
                        let box_pad_y = 4.0;
                        renderer.draw_debug_box(
                            Vec2::new(x - box_pad_x, y - box_pad_y),
                            Vec2::new(w + box_pad_x * 2.0, 8.0 * opt_scale + box_pad_y * 2.0),
                            [0.25, 0.04, 0.04, 0.65],
                            [0.95, 0.20, 0.20, 0.90],
                        );
                        renderer.draw_debug_text(
                            &label,
                            Vec2::new(x, y),
                            [1.0, 0.95, 0.95, 1.0],
                            opt_scale,
                        );
                    } else {
                        renderer.draw_debug_text(
                            &label,
                            Vec2::new(x, y),
                            [0.70, 0.70, 0.78, 0.85],
                            opt_scale,
                        );
                    }
                }

                // Footer Controls Prompt
                let footer = "[W / S] Select  |  [ENTER / SPACE] Confirm  |  [ESC] Resume";
                let foot_scale = 0.8;
                let foot_w = footer.len() as f32 * 8.0 * foot_scale;
                let foot_x = (viewport_size.x - foot_w) * 0.5;
                renderer.draw_debug_text(
                    footer,
                    Vec2::new(foot_x, viewport_size.y - 30.0),
                    [0.65, 0.65, 0.75, 0.85],
                    foot_scale,
                );
            }
        }
    }
}

