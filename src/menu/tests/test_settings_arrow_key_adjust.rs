use glam::Vec2;
use crate::engine::InputState;
use crate::menu::{MenuScreen, MenuSystem, TimestepMode};

//------------[Verify Keyboard Left and Right Keys Adjust Setting Options]-------------------
#[test]
fn test_settings_arrow_key_adjust() {
    let mut menu = MenuSystem::new();
    menu.screen = MenuScreen::Settings;
    menu.settings.timestep_mode = TimestepMode::Hz60;
    // TimestepMode is at visible index 4 (WindowMode, VSync, MatchTimestep/FpsLock, TimestepMode...)
    let visible = menu.visible_settings();
    let timestep_idx = visible
        .iter()
        .position(|&item| item == crate::menu::SettingItem::TimestepMode)
        .unwrap_or(0);
    menu.settings_selected = timestep_idx;

    // 1. Press Left arrow key -> cycles to prev (Hz60 -> Hz30)
    let mut input_left = InputState::new();
    input_left.on_key_event(
        winit::keyboard::PhysicalKey::Code(winit::keyboard::KeyCode::ArrowLeft),
        winit::event::ElementState::Pressed,
    );
    menu.update(&input_left, Vec2::new(640.0, 360.0), Vec2::new(640.0, 360.0));
    assert_eq!(menu.settings.timestep_mode, TimestepMode::Hz30);

    // 2. Press Right arrow key -> cycles to next (Hz30 -> Hz60)
    let mut input_right = InputState::new();
    input_right.on_key_event(
        winit::keyboard::PhysicalKey::Code(winit::keyboard::KeyCode::ArrowRight),
        winit::event::ElementState::Pressed,
    );
    menu.update(&input_right, Vec2::new(640.0, 360.0), Vec2::new(640.0, 360.0));
    assert_eq!(menu.settings.timestep_mode, TimestepMode::Hz60);
}

