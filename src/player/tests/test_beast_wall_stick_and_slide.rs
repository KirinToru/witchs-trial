use glam::Vec2;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::InputState;
use crate::player::{Player, PlayerForm};

//------------[Verify The Beast Sticks In Place On Wall and Slides With Down Key]-------------------
#[test]
fn test_beast_wall_stick_and_slide() {
    let mut player = Player::new_headless(Vec2::new(100.0, 100.0));
    player.form = PlayerForm::Beast;
    player.grounded = false;
    player.walled_right = true;
    player.vel.y = 80.0; // Falling velocity

    // Press towards the wall (Right / D) without Down key
    let mut input = InputState::new();
    input.on_key_event(PhysicalKey::Code(KeyCode::KeyD), winit::event::ElementState::Pressed);

    player.update(0.02, &input, &[], &[], &[], &[], &[], &mut []);

    // Beast mode sticks in place on wall without falling
    assert_eq!(player.vel.y, 0.0);

    // Now hold Down (KeyS) while still holding towards wall
    input.on_key_event(PhysicalKey::Code(KeyCode::KeyS), winit::event::ElementState::Pressed);

    player.update(0.02, &input, &[], &[], &[], &[], &[], &mut []);

    // When holding Down, player slides down along wall
    assert!(player.vel.y > 0.0);
}
