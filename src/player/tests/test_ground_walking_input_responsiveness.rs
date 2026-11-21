use glam::Vec2;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::InputState;
use crate::player::Player;

//------------[Verify Grounded Player Accelerates Normally When Pressing Directional Keys]-------------------
#[test]
fn test_ground_walking_input_responsiveness() {
    let mut player = Player::new_headless(Vec2::new(100.0, 100.0));
    player.grounded = true;

    let mut input = InputState::new();
    input.on_key_event(PhysicalKey::Code(KeyCode::KeyD), winit::event::ElementState::Pressed);

    let dt = 1.0 / 60.0;
    player.update(dt, &input, &[], &[], &[], &mut []);

    // Acceleration is 1800 px/s^2, so after 1 tick at 60Hz speed should be approx 30 px/s
    assert!(player.vel.x > 0.0);
    assert!((player.vel.x - 30.0).abs() < 0.1);
}
