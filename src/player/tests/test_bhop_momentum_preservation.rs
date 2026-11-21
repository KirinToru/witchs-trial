use glam::Vec2;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::InputState;
use crate::player::Player;

//------------[Verify BHOP Momentum Preserved Without Ground Friction During Active Window]-------------------
#[test]
fn test_bhop_momentum_preservation() {
    let mut player = Player::new_headless(Vec2::new(100.0, 100.0));
    player.grounded = true;
    player.is_bhopping = true;
    player.bhop_timer = 0.15;
    player.bhop_velocity = 280.0;
    player.vel.x = 280.0;

    let mut input = InputState::new();
    input.on_key_event(PhysicalKey::Code(KeyCode::KeyD), winit::event::ElementState::Pressed);

    player.update(1.0 / 60.0, &input, &[], &[], &[], &[], &mut []);

    // Full velocity preserved without ground friction deceleration
    assert!(player.is_bhopping);
    assert_eq!(player.vel.x, 280.0);
}
