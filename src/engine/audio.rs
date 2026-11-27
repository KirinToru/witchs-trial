//! Embedded/disk-hybrid audio subsystem for looping music tracks and cached SFX playback.

use std::collections::HashMap;
use std::io::Cursor;
use std::sync::Arc;
use std::time::{Duration, Instant};
use rodio::{Decoder, DeviceSinkBuilder, MixerDeviceSink, Player as AudioPlayer, Source};
use crate::engine::assets;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum MusicTrack {
    Menu,
    Level,
}

impl MusicTrack {
    //------------[Resolve music stem name]-------------------
    fn file_stem(self) -> &'static str {
        match self {
            Self::Menu => "menu",
            Self::Level => "level",
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum SfxEvent {
    Jump,
    WallJump,
    Land,
    Dash,
    BeastTransform,
    BeastHurt,
    Death,
    Checkpoint,
    CrateDash,
    LevelComplete,
    MenuMove,
    MenuConfirm,
}

impl SfxEvent {
    const ALL: &'static [SfxEvent] = &[
        Self::Jump,
        Self::WallJump,
        Self::Land,
        Self::Dash,
        Self::BeastTransform,
        Self::BeastHurt,
        Self::Death,
        Self::Checkpoint,
        Self::CrateDash,
        Self::LevelComplete,
        Self::MenuMove,
        Self::MenuConfirm,
    ];

    //------------[Resolve SFX stem name]-------------------
    fn file_stem(self) -> &'static str {
        match self {
            Self::Jump => "jump",
            Self::WallJump => "wall_jump",
            Self::Land => "land",
            Self::Dash => "dash",
            Self::BeastTransform => "beast_transform",
            Self::BeastHurt => "beast_hurt",
            Self::Death => "death",
            Self::Checkpoint => "checkpoint",
            Self::CrateDash => "crate_dash",
            Self::LevelComplete => "level_complete",
            Self::MenuMove => "menu_move",
            Self::MenuConfirm => "menu_confirm",
        }
    }
}

// Wrapper so Arc<Vec<u8>> satisfies Cursor's AsRef<[u8]> requirement without copying bytes
#[derive(Clone)]
struct SharedAudioBytes(Arc<Vec<u8>>);

impl AsRef<[u8]> for SharedAudioBytes {
    fn as_ref(&self) -> &[u8] {
        &self.0
    }
}

pub struct AudioSystem {
    sink: Option<MixerDeviceSink>,
    music_player: Option<AudioPlayer>,
    sfx_cache: HashMap<SfxEvent, Arc<Vec<u8>>>,
    last_sfx_played: HashMap<SfxEvent, Instant>,
    current_track: Option<MusicTrack>,
    music_paused: bool,
    music_volume: f32,
    sfx_volume: f32,
}

impl AudioSystem {
    //------------[Load first existing audio bytes from disk or embedded assets]-------------------
    fn load_audio_bytes(dir: &str, stem: &str, exts: &[&str]) -> Option<Vec<u8>> {
        for ext in exts {
            let candidate = format!("{}/{}.{}", dir, stem, ext);
            if let Ok(bytes) = assets::load_bytes(&candidate) {
                return Some(bytes);
            }
        }
        None
    }

    //------------[Preload all SFX files into memory cache]-------------------
    fn load_sfx_cache() -> HashMap<SfxEvent, Arc<Vec<u8>>> {
        let mut cache = HashMap::new();
        for &event in SfxEvent::ALL {
            let stem = event.file_stem();
            // SFX prefers WAV first, then OGG
            if let Some(bytes) = Self::load_audio_bytes("assets/audio/sfx", stem, &["wav", "ogg"]) {
                cache.insert(event, Arc::new(bytes));
            } else {
                log::warn!(
                    "Missing SFX asset for {:?}: expected assets/audio/sfx/{}.wav or .ogg",
                    event,
                    stem
                );
            }
        }
        cache
    }

    //------------[Initialize audio system with safe silent fallback]-------------------
    pub fn new() -> Self {
        let sink = match DeviceSinkBuilder::open_default_sink() {
            Ok(mut device_sink) => {
                device_sink.log_on_drop(false);
                Some(device_sink)
            }
            Err(err) => {
                log::warn!("Audio device unavailable, running in silent mode: {}", err);
                None
            }
        };

        let sfx_cache = Self::load_sfx_cache();

        Self {
            sink,
            music_player: None,
            sfx_cache,
            last_sfx_played: HashMap::new(),
            current_track: None,
            music_paused: false,
            music_volume: 0.7,
            sfx_volume: 0.8,
        }
    }

    //------------[Create silent headless audio system for unit tests]-------------------
    #[allow(dead_code)]
    pub fn new_headless() -> Self {
        Self {
            sink: None,
            music_player: None,
            sfx_cache: HashMap::new(),
            last_sfx_played: HashMap::new(),
            current_track: None,
            music_paused: false,
            music_volume: 0.7,
            sfx_volume: 0.8,
        }
    }

    //------------[Start looping music track if not already playing]-------------------
    pub fn play_music(&mut self, track: MusicTrack) {
        if self.current_track == Some(track) {
            return;
        }
        self.current_track = Some(track);

        // Stop any active previous track cleanly
        if let Some(old_player) = self.music_player.take() {
            old_player.stop();
        }

        let Some(sink) = &self.sink else {
            return;
        };

        // Music prefers OGG first, then WAV
        let stem = track.file_stem();
        let Some(bytes) = Self::load_audio_bytes("assets/audio/music", stem, &["ogg", "wav"]) else {
            log::warn!(
                "Missing music asset for {:?}: expected assets/audio/music/{}.ogg or .wav",
                track,
                stem
            );
            return;
        };

        let len = bytes.len() as u64;
        let cursor = Cursor::new(bytes);
        match Decoder::builder()
            .with_data(cursor)
            .with_byte_len(len)
            .with_seekable(true)
            .build()
        {
            Ok(decoder) => {
                let player = AudioPlayer::connect_new(sink.mixer());
                player.set_volume(self.music_volume);
                if self.music_paused {
                    player.pause();
                }
                player.append(decoder.repeat_infinite());
                self.music_player = Some(player);
            }
            Err(err) => {
                log::warn!("Failed to decode music asset for {:?}: {}", track, err);
            }
        }
    }

    //------------[Pause or resume active music playback]-------------------
    pub fn set_music_paused(&mut self, paused: bool) {
        if self.music_paused == paused {
            return;
        }
        self.music_paused = paused;
        if let Some(player) = &self.music_player {
            if paused {
                player.pause();
            } else {
                player.play();
            }
        }
    }

    //------------[Play cached sound effect with per-event cooldown and volume]-------------------
    pub fn play_sfx(&mut self, event: SfxEvent) {
        if self.sfx_volume <= 0.0 {
            return;
        }

        // Enforce per-event cooldown so repeated contacts never flood the audio mixer
        let min_cooldown = match event {
            SfxEvent::CrateDash | SfxEvent::Land => Duration::from_millis(120),
            _ => Duration::from_millis(60),
        };
        let now = Instant::now();
        if let Some(&last_time) = self.last_sfx_played.get(&event) {
            if now.duration_since(last_time) < min_cooldown {
                return;
            }
        }

        let Some(sink) = &self.sink else {
            return;
        };
        let Some(bytes) = self.sfx_cache.get(&event) else {
            return;
        };

        let len = bytes.len() as u64;
        let cursor = Cursor::new(SharedAudioBytes(Arc::clone(bytes)));
        match Decoder::builder()
            .with_data(cursor)
            .with_byte_len(len)
            .with_seekable(true)
            .build()
        {
            Ok(decoder) => {
                self.last_sfx_played.insert(event, now);
                sink.mixer().add(decoder.amplify(self.sfx_volume));
            }
            Err(err) => {
                log::warn!("Failed to decode cached SFX {:?}: {}", event, err);
            }
        }
    }

    //------------[Set music volume from 0-100 percentage]-------------------
    pub fn set_music_volume(&mut self, percent: u8) {
        self.music_volume = (percent.min(100) as f32) / 100.0;
        if let Some(player) = &self.music_player {
            player.set_volume(self.music_volume);
        }
    }

    //------------[Set sound effects volume from 0-100 percentage]-------------------
    pub fn set_sfx_volume(&mut self, percent: u8) {
        self.sfx_volume = (percent.min(100) as f32) / 100.0;
    }
}
