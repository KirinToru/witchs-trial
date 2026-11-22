use glam::Vec2;
use crate::engine::InputState;
use crate::menu::{GameSettings, MenuScreen, MenuSystem, SettingItem};

//------------[Verify PSX Shader Setting Default, Visibility, Toggle Keys, And No Restart Required]-------------------
#[test]
fn test_psx_shader_setting_toggle() {
    // 1. Default value in GameSettings must be true
    assert!(GameSettings::default().psx_shader);

    let mut menu = MenuSystem::new();
    menu.settings.psx_shader = true;
    menu.screen = MenuScreen::Settings;

    // 2. SettingItem::PsxShader must be present in visible settings
    let visible = menu.visible_settings();
    assert!(visible.contains(&SettingItem::PsxShader));

    let psx_idx = visible
        .iter()
        .position(|&item| item == SettingItem::PsxShader)
        .expect("PsxShader must be in visible settings");
    menu.settings_selected = psx_idx;

    // 3. Left arrow toggles setting from true to false
    let mut input_left = InputState::new();
    input_left.on_key_event(
        winit::keyboard::PhysicalKey::Code(winit::keyboard::KeyCode::ArrowLeft),
        winit::event::ElementState::Pressed,
    );
    menu.update(&input_left, Vec2::new(640.0, 360.0), Vec2::new(640.0, 360.0));
    assert!(!menu.settings.psx_shader);
    assert!(menu.settings_changed);

    // 4. Right arrow toggles setting from false to true
    let mut input_right = InputState::new();
    input_right.on_key_event(
        winit::keyboard::PhysicalKey::Code(winit::keyboard::KeyCode::ArrowRight),
        winit::event::ElementState::Pressed,
    );
    menu.update(&input_right, Vec2::new(640.0, 360.0), Vec2::new(640.0, 360.0));
    assert!(menu.settings.psx_shader);
    assert!(menu.settings_changed);

    // 5. Enter (Confirm) toggles setting from true to false
    let mut input_confirm = InputState::new();
    input_confirm.on_key_event(
        winit::keyboard::PhysicalKey::Code(winit::keyboard::KeyCode::Enter),
        winit::event::ElementState::Pressed,
    );
    menu.update(&input_confirm, Vec2::new(640.0, 360.0), Vec2::new(640.0, 360.0));
    assert!(!menu.settings.psx_shader);
    assert!(menu.settings_changed);

    // 6. Changing PSX shader does not trigger restart requirement notice
    assert_eq!(menu.settings.graphics_backend, menu.initial_backend);
}
