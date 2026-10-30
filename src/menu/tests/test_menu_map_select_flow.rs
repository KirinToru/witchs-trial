use glam::Vec2;
use crate::engine::InputState;
use crate::menu::{MenuScreen, MenuSystem};

//------------[Verify Menu Navigation and Map Selection Flow]-------------------
#[test]
fn test_menu_map_select_flow() {
    let mut menu = MenuSystem::new();
    let mut input = InputState::new();

    assert_eq!(menu.screen, MenuScreen::Main);
    assert_eq!(menu.main_selected, 0);

    // Press confirm on Start Game -> transitions to MapSelect
    input.on_key_event(
        winit::keyboard::PhysicalKey::Code(winit::keyboard::KeyCode::Enter),
        winit::event::ElementState::Pressed,
    );
    menu.update(&input, Vec2::new(1280.0, 720.0), Vec2::new(640.0, 360.0));

    assert_eq!(menu.screen, MenuScreen::MapSelect);
    assert!(!menu.request_start_game);

    // Ensure available maps has a test map
    if menu.available_maps.is_empty() {
        menu.available_maps.push("test.tmx".to_string());
    }

    // Confirm map selection -> triggers request_start_game with map path
    let expected_map = format!("assets/maps/{}", menu.available_maps[0]);
    menu.update(&input, Vec2::new(1280.0, 720.0), Vec2::new(640.0, 360.0));
    assert!(menu.request_start_game);
    assert_eq!(menu.selected_map_path, Some(expected_map));
}

