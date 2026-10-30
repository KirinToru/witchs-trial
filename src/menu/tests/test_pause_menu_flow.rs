use crate::engine::InputState;
use crate::menu::{MenuScreen, MenuSystem};
use glam::Vec2;
use winit::event::ElementState;
use winit::keyboard::{KeyCode, PhysicalKey};

fn press_key(key: KeyCode) -> InputState {
    let mut input = InputState::new();
    input.on_key_event(PhysicalKey::Code(key), ElementState::Pressed);
    input
}

//------------[Verify Pause Menu Navigation Selection and Resume Flow]-------------------
#[test]
fn test_pause_menu_flow() {
    let mut menu = MenuSystem::new();
    let viewport = Vec2::new(640.0, 360.0);

    // 1. Enter Pause Menu
    menu.screen = MenuScreen::Pause;
    menu.pause_selected = 0;

    // 2. Down navigation wraps: 0 (Continue) -> 1 (Settings) -> 2 (Main Menu) -> 0
    menu.update(&press_key(KeyCode::ArrowDown), viewport, viewport);
    assert_eq!(menu.pause_selected, 1);

    menu.update(&press_key(KeyCode::ArrowDown), viewport, viewport);
    assert_eq!(menu.pause_selected, 2);

    menu.update(&press_key(KeyCode::ArrowDown), viewport, viewport);
    assert_eq!(menu.pause_selected, 0);

    // 3. Confirming index 0 (Continue) sets request_resume_game
    menu.update(&press_key(KeyCode::Enter), viewport, viewport);
    assert!(menu.request_resume_game);

    // 4. Confirming index 1 (Settings) opens Settings with return to Pause
    menu.screen = MenuScreen::Pause;
    menu.pause_selected = 1;
    menu.update(&press_key(KeyCode::Enter), viewport, viewport);
    assert_eq!(menu.screen, MenuScreen::Settings);
    assert_eq!(menu.settings_return_screen, MenuScreen::Pause);

    // 5. Escape in Settings returns back to Pause
    menu.update(&press_key(KeyCode::Escape), viewport, viewport);
    assert_eq!(menu.screen, MenuScreen::Pause);

    // 6. Escape in Pause resumes the game
    menu.update(&press_key(KeyCode::Escape), viewport, viewport);
    assert!(menu.request_resume_game);

    // 7. Selecting index 2 (Main Menu) returns to Main Menu
    menu.screen = MenuScreen::Pause;
    menu.pause_selected = 2;
    menu.update(&press_key(KeyCode::Enter), viewport, viewport);
    assert_eq!(menu.screen, MenuScreen::Main);
}
