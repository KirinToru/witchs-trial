use glam::Vec2;
use crate::engine::InputState;
use crate::menu::{GameSettings, MenuScreen, MenuSystem, SettingItem};

//------------[Verify Voxel Shader Setting Default, Visibility, Toggle Keys, And No Restart Required]-------------------
#[test]
fn test_voxel_shader_setting_toggle() {
    // 1. Default value in GameSettings must be true
    assert!(GameSettings::default().voxel_shader);

    let mut menu = MenuSystem::new();
    menu.settings.voxel_shader = true;
    menu.screen = MenuScreen::Settings;

    // 2. SettingItem::VoxelShader must be present in visible settings
    let visible = menu.visible_settings();
    assert!(visible.contains(&SettingItem::VoxelShader));

    let voxel_idx = visible
        .iter()
        .position(|&item| item == SettingItem::VoxelShader)
        .expect("VoxelShader must be in visible settings");
    menu.settings_selected = voxel_idx;

    // 3. Left arrow toggles setting from true to false
    let mut input_left = InputState::new();
    input_left.on_key_event(
        winit::keyboard::PhysicalKey::Code(winit::keyboard::KeyCode::ArrowLeft),
        winit::event::ElementState::Pressed,
    );
    menu.update(&input_left, Vec2::new(640.0, 360.0), Vec2::new(640.0, 360.0));
    assert!(!menu.settings.voxel_shader);
    assert!(menu.settings_changed);

    // 4. Right arrow toggles setting from false to true
    let mut input_right = InputState::new();
    input_right.on_key_event(
        winit::keyboard::PhysicalKey::Code(winit::keyboard::KeyCode::ArrowRight),
        winit::event::ElementState::Pressed,
    );
    menu.update(&input_right, Vec2::new(640.0, 360.0), Vec2::new(640.0, 360.0));
    assert!(menu.settings.voxel_shader);
    assert!(menu.settings_changed);

    // 5. Enter (Confirm) toggles setting from true to false
    let mut input_confirm = InputState::new();
    input_confirm.on_key_event(
        winit::keyboard::PhysicalKey::Code(winit::keyboard::KeyCode::Enter),
        winit::event::ElementState::Pressed,
    );
    menu.update(&input_confirm, Vec2::new(640.0, 360.0), Vec2::new(640.0, 360.0));
    assert!(!menu.settings.voxel_shader);
    assert!(menu.settings_changed);

    // 6. Changing voxel shader does not trigger restart requirement notice
    assert_eq!(menu.settings.graphics_backend, menu.initial_backend);
}
