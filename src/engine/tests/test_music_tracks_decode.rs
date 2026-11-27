use std::io::Cursor;
use std::num::NonZero;
use rodio::{Decoder, Player};
use crate::engine::assets;

//------------[Verify Embedded Music Tracks Decode and Output Samples Headlessly]-------------------
#[test]
fn test_music_tracks_decode() {
    // 1. Load and decode bundled Menu music via looped decoder
    let menu_bytes = assets::load_bytes("assets/audio/music/menu.wav")
        .expect("menu music asset should load from disk or embedded bundle");
    let menu_decoder = Decoder::new_looped(Cursor::new(menu_bytes.clone()))
        .expect("menu music should decode with Decoder::new_looped");
    let menu_samples: Vec<f32> = menu_decoder.take(4096).collect();
    assert!(!menu_samples.is_empty());
    assert!(menu_samples.iter().any(|&s| s.abs() > 1e-4));

    // 2. Load and decode bundled Level music via both standard and looped decoders
    let level_bytes = assets::load_bytes("assets/audio/music/level.ogg")
        .expect("level music asset should load from disk or embedded bundle");
    let level_single = Decoder::new(Cursor::new(level_bytes.clone()))
        .expect("level.ogg should decode with Decoder::new");
    let single_samples: Vec<f32> = level_single.take(4096).collect();
    assert!(!single_samples.is_empty());
    assert!(single_samples.iter().any(|&s| s.abs() > 1e-4));

    // 3. Verify looped OGG playback feeds non-zero audio through Player and headless mixer
    let (mixer, mut rx) = rodio::mixer::mixer(NonZero::new(2).unwrap(), NonZero::new(48000).unwrap());
    let (player, queue_rx) = Player::new();
    let level_looped = Decoder::new_looped(Cursor::new(level_bytes))
        .expect("level.ogg should decode with Decoder::new_looped");
    player.append(level_looped);
    mixer.add(queue_rx);

    let mut mixed_samples = Vec::with_capacity(8192);
    for _ in 0..8192 {
        mixed_samples.push(rx.next().unwrap_or(0.0));
    }
    assert!(mixed_samples.iter().any(|&s| s.abs() > 1e-4));
}
