use glam::Vec2;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::InputState;
use crate::player::Player;

//------------[Verify Wall Jump Lock Cancellation When Steering Back Into Wall]-------------------
#[test]
fn test_wall_jump_lock_cancellation() {
    let mut player = Player::new_headless(Vec2::new(100.0, 100.0));
    player.wall_jump_lock_timer = 0.08;
    player.vel.x = 250.0; // Moving right after wall jump

    let mut input = InputState::new();
    // Steering left (opposite direction to velocity) cancels the lock timer immediately
    input.on_key_event(PhysicalKey::Code(KeyCode::KeyA), winit::event::ElementState::Pressed);

    player.update(0.01, &input, &[], &[], &[], &[], &mut []);
    assert_eq!(player.wall_jump_lock_timer, 0.0);
}
