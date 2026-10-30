use crate::engine::GraphicsBackendOption;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum WindowDisplayMode {
    Windowed,
    Borderless,
    Exclusive,
}

impl WindowDisplayMode {
    //------------[Window mode label]-------------------
    pub fn label(self) -> &'static str {
        match self {
            Self::Windowed => "Windowed",
            Self::Borderless => "Borderless Fullscreen",
            Self::Exclusive => "Exclusive Fullscreen",
        }
    }

    //------------[Cycle next window mode]-------------------
    pub fn next(self) -> Self {
        match self {
            Self::Windowed => Self::Borderless,
            Self::Borderless => Self::Exclusive,
            Self::Exclusive => Self::Windowed,
        }
    }

    //------------[Cycle previous window mode]-------------------
    pub fn prev(self) -> Self {
        match self {
            Self::Windowed => Self::Exclusive,
            Self::Borderless => Self::Windowed,
            Self::Exclusive => Self::Borderless,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FpsLockOption {
    Unlimited,
    Fps30,
    Fps60,
    Fps90,
    Fps120,
    Fps144,
    Fps165,
    Fps180,
    Fps240,
    Fps300,
}

impl FpsLockOption {
    pub const ALL: &'static [FpsLockOption] = &[
        FpsLockOption::Unlimited,
        FpsLockOption::Fps30,
        FpsLockOption::Fps60,
        FpsLockOption::Fps90,
        FpsLockOption::Fps120,
        FpsLockOption::Fps144,
        FpsLockOption::Fps165,
        FpsLockOption::Fps180,
        FpsLockOption::Fps240,
        FpsLockOption::Fps300,
    ];

    //------------[Get target FPS value]-------------------
    pub fn target_fps(self) -> Option<u32> {
        match self {
            Self::Unlimited => None,
            Self::Fps30 => Some(30),
            Self::Fps60 => Some(60),
            Self::Fps90 => Some(90),
            Self::Fps120 => Some(120),
            Self::Fps144 => Some(144),
            Self::Fps165 => Some(165),
            Self::Fps180 => Some(180),
            Self::Fps240 => Some(240),
            Self::Fps300 => Some(300),
        }
    }

    //------------[FPS lock label]-------------------
    pub fn label(self) -> &'static str {
        match self {
            Self::Unlimited => "Unlimited",
            Self::Fps30 => "30 FPS",
            Self::Fps60 => "60 FPS",
            Self::Fps90 => "90 FPS",
            Self::Fps120 => "120 FPS",
            Self::Fps144 => "144 FPS",
            Self::Fps165 => "165 FPS",
            Self::Fps180 => "180 FPS",
            Self::Fps240 => "240 FPS",
            Self::Fps300 => "300 FPS",
        }
    }

    //------------[Cycle next FPS lock]-------------------
    pub fn next(self) -> Self {
        let all = Self::ALL;
        let idx = all.iter().position(|&f| f == self).unwrap_or(0);
        all[(idx + 1) % all.len()]
    }

    //------------[Cycle previous FPS lock]-------------------
    pub fn prev(self) -> Self {
        let all = Self::ALL;
        let idx = all.iter().position(|&f| f == self).unwrap_or(0);
        if idx == 0 {
            all[all.len() - 1]
        } else {
            all[idx - 1]
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum TimestepMode {
    Dynamic,
    Hz30,
    Hz60,
    Hz90,
    Hz120,
    Hz144,
    Hz165,
    Hz180,
    Hz240,
    Hz300,
}

impl TimestepMode {
    pub const ALL: &'static [TimestepMode] = &[
        TimestepMode::Dynamic,
        TimestepMode::Hz30,
        TimestepMode::Hz60,
        TimestepMode::Hz90,
        TimestepMode::Hz120,
        TimestepMode::Hz144,
        TimestepMode::Hz165,
        TimestepMode::Hz180,
        TimestepMode::Hz240,
        TimestepMode::Hz300,
    ];

    //------------[Timestep mode label]-------------------
    pub fn label(self) -> &'static str {
        match self {
            Self::Dynamic => "Dynamic",
            Self::Hz30 => "30 Hz",
            Self::Hz60 => "60 Hz",
            Self::Hz90 => "90 Hz",
            Self::Hz120 => "120 Hz",
            Self::Hz144 => "144 Hz",
            Self::Hz165 => "165 Hz",
            Self::Hz180 => "180 Hz",
            Self::Hz240 => "240 Hz",
            Self::Hz300 => "300 Hz",
        }
    }

    //------------[Get timestep frequency in Hz]-------------------
    pub fn hz(self) -> Option<f32> {
        match self {
            Self::Dynamic => None,
            Self::Hz30 => Some(30.0),
            Self::Hz60 => Some(60.0),
            Self::Hz90 => Some(90.0),
            Self::Hz120 => Some(120.0),
            Self::Hz144 => Some(144.0),
            Self::Hz165 => Some(165.0),
            Self::Hz180 => Some(180.0),
            Self::Hz240 => Some(240.0),
            Self::Hz300 => Some(300.0),
        }
    }

    //------------[Cycle next timestep mode]-------------------
    pub fn next(self) -> Self {
        let all = Self::ALL;
        let idx = all.iter().position(|&m| m == self).unwrap_or(0);
        all[(idx + 1) % all.len()]
    }

    //------------[Cycle previous timestep mode]-------------------
    pub fn prev(self) -> Self {
        let all = Self::ALL;
        let idx = all.iter().position(|&m| m == self).unwrap_or(0);
        if idx == 0 {
            all[all.len() - 1]
        } else {
            all[idx - 1]
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SettingItem {
    WindowMode,
    VSync,
    MatchTimestep,
    FpsLock,
    TimestepMode,
    AutoBhop,
    GraphicsBackend,
    Back,
}

#[derive(Debug, Clone, Copy)]
pub struct LevelStats {
    pub time_secs: f32,
    pub deaths: u32,
    pub jumps: u32,
    pub dashes: u32,
}

#[derive(Debug, Clone)]
pub struct GameSettings {
    pub window_mode: WindowDisplayMode,
    pub fps_lock: FpsLockOption,
    pub vsync: bool,
    pub match_timestep: bool,
    pub timestep_mode: TimestepMode,
    pub auto_bhop: bool,
    pub graphics_backend: GraphicsBackendOption,
}

impl Default for GameSettings {
    //------------[Default game settings]-------------------
    fn default() -> Self {
        Self {
            window_mode: WindowDisplayMode::Windowed,
            fps_lock: FpsLockOption::Fps60,
            vsync: true,
            match_timestep: false,
            timestep_mode: TimestepMode::Dynamic,
            auto_bhop: true,
            graphics_backend: GraphicsBackendOption::Auto,
        }
    }
}

impl GameSettings {
    //------------[Load settings or use defaults]-------------------
    pub fn load_or_default() -> Self {
        let mut settings = Self::default();
        if let Ok(content) = std::fs::read_to_string("settings.cfg") {
            for line in content.lines() {
                let trimmed = line.trim();
                if trimmed.is_empty() || trimmed.starts_with('#') {
                    continue;
                }
                if let Some((key, val)) = trimmed.split_once('=') {
                    let (k, v) = (key.trim(), val.trim());
                    match k {
                        "window_mode" => {
                            settings.window_mode = match v {
                                "Borderless" => WindowDisplayMode::Borderless,
                                "Exclusive" => WindowDisplayMode::Exclusive,
                                _ => WindowDisplayMode::Windowed,
                            };
                        }
                        "vsync" => {
                            settings.vsync = v.eq_ignore_ascii_case("true");
                        }
                        "match_timestep" => {
                            settings.match_timestep = v.eq_ignore_ascii_case("true");
                        }
                        "fps_lock" => {
                            settings.fps_lock = match v {
                                "Unlimited" => FpsLockOption::Unlimited,
                                "Fps30" => FpsLockOption::Fps30,
                                "Fps60" => FpsLockOption::Fps60,
                                "Fps90" => FpsLockOption::Fps90,
                                "Fps120" => FpsLockOption::Fps120,
                                "Fps144" => FpsLockOption::Fps144,
                                "Fps165" => FpsLockOption::Fps165,
                                "Fps180" => FpsLockOption::Fps180,
                                "Fps240" => FpsLockOption::Fps240,
                                "Fps300" => FpsLockOption::Fps300,
                                _ => FpsLockOption::Fps60,
                            };
                        }
                        "timestep_mode" => {
                            settings.timestep_mode = match v {
                                "Dynamic" => TimestepMode::Dynamic,
                                "Hz30" => TimestepMode::Hz30,
                                "Hz60" => TimestepMode::Hz60,
                                "Hz90" => TimestepMode::Hz90,
                                "Hz120" => TimestepMode::Hz120,
                                "Hz144" => TimestepMode::Hz144,
                                "Hz165" => TimestepMode::Hz165,
                                "Hz180" => TimestepMode::Hz180,
                                "Hz240" => TimestepMode::Hz240,
                                "Hz300" => TimestepMode::Hz300,
                                _ => TimestepMode::Dynamic,
                            };
                        }
                        "auto_bhop" => {
                            settings.auto_bhop = v.eq_ignore_ascii_case("true");
                        }
                        "graphics_backend" => {
                            settings.graphics_backend = match v {
                                "Vulkan" => GraphicsBackendOption::Vulkan,
                                "DirectX12" => GraphicsBackendOption::DirectX12,
                                "OpenGL" => GraphicsBackendOption::OpenGL,
                                "Metal" => GraphicsBackendOption::Metal,
                                _ => GraphicsBackendOption::Auto,
                            };
                        }
                        _ => {}
                    }
                }
            }
        }
        settings
    }

    //------------[Save settings to file]-------------------
    pub fn save(&self) {
        let content = format!(
            "window_mode={:?}\nvsync={}\nmatch_timestep={}\nfps_lock={:?}\ntimestep_mode={:?}\nauto_bhop={}\ngraphics_backend={:?}\n",
            self.window_mode,
            self.vsync,
            self.match_timestep,
            self.fps_lock,
            self.timestep_mode,
            self.auto_bhop,
            self.graphics_backend
        );
        let _ = std::fs::write("settings.cfg", content);
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MenuScreen {
    Main,
    MapSelect,
    Settings,
    LevelComplete,
    Pause,
}

