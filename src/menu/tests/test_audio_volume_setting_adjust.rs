use glam::Vec2;
use winit::event::ElementState;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::{InputState, SfxEvent};
use crate::menu::{MenuScreen, MenuSystem, SettingItem};

fn press_key(key: KeyCode) -> InputState {
    let mut input = InputState::new();
    input.on_key_event(PhysicalKey::Code(key), ElementState::Pressed);
    input
}

//------------[Verify Audio Volume Setting Adjustment Without Game Restart]-------------------
#[test]
fn test_audio_volume_setting_adjust() {
    let mut menu = MenuSystem::new();
    menu.screen = MenuScreen::Settings;
    menu.settings.music_volume = 10;
    menu.settings.sfx_volume = 90;

    let visible = menu.visible_settings();
    let music_idx = visible
        .iter()
        .position(|&item| item == SettingItem::MusicVolume)
        .expect("MusicVolume must be in visible settings");
    let sfx_idx = visible
        .iter()
        .position(|&item| item == SettingItem::SfxVolume)
        .expect("SfxVolume must be in visible settings");

    let viewport = Vec2::new(640.0, 360.0);

    // 1. MusicVolume Left clamps at 0%, Right increases, Enter wraps 100% -> 0%
    menu.settings_selected = music_idx;
    menu.update(&press_key(KeyCode::ArrowLeft), viewport, viewport);
    assert_eq!(menu.settings.music_volume, 0);
    assert!(menu.settings_changed);
    assert!(menu.pending_sfx.contains(&SfxEvent::MenuMove));

    menu.update(&press_key(KeyCode::ArrowLeft), viewport, viewport);
    assert_eq!(menu.settings.music_volume, 0);

    menu.settings.music_volume = 100;
    menu.update(&press_key(KeyCode::ArrowRight), viewport, viewport);
    assert_eq!(menu.settings.music_volume, 100);

    menu.update(&press_key(KeyCode::Enter), viewport, viewport);
    assert_eq!(menu.settings.music_volume, 0);
    assert!(menu.pending_sfx.contains(&SfxEvent::MenuConfirm));

    // 2. SfxVolume Right increases by 10% up to 100%, Enter wraps to 0%
    menu.settings_selected = sfx_idx;
    menu.update(&press_key(KeyCode::ArrowRight), viewport, viewport);
    assert_eq!(menu.settings.sfx_volume, 100);
    assert!(menu.settings_changed);

    menu.update(&press_key(KeyCode::Enter), viewport, viewport);
    assert_eq!(menu.settings.sfx_volume, 0);

    // 3. Changing audio volumes must not require game restart
    assert_eq!(menu.settings.graphics_backend, menu.initial_backend);
}
