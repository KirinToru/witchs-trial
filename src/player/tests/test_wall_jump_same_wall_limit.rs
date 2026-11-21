use glam::Vec2;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::InputState;
use crate::player::Player;

//------------[Verify Second Wall Jump Off Same Wall In Air Is Blocked]-------------------
#[test]
fn test_wall_jump_same_wall_limit() {
    let mut player = Player::new_headless(Vec2::new(100.0, 100.0));
    player.grounded = false;
    player.walled_left = true;

    let mut input = InputState::new();
    input.on_key_event(PhysicalKey::Code(KeyCode::Space), winit::event::ElementState::Pressed);

    // 1. First jump off left wall succeeds (initial vertical impulse -500 px/s with gravity)
    player.update(0.01, &input, &[], &[], &[], &[], &mut []);
    assert_eq!(player.jumps_count, 1);
    assert_eq!(player.last_wall_dir, Some(-1));
    assert_eq!(player.vel.x, 250.0);
    assert!(player.vel.y < -400.0);

    // 2. Player remains in air and touches the same left wall again
    player.grounded = false;
    player.walled_left = true;
    player.walled_right = false;

    // Request jump again on the same left wall
    input.clear_frame_edges();
    input.on_key_event(PhysicalKey::Code(KeyCode::Space), winit::event::ElementState::Pressed);

    // Advance simulation tick
    player.update(0.01, &input, &[], &[], &[], &[], &mut []);

    // Second jump off the same wall must be blocked
    assert_eq!(player.jumps_count, 1);
    assert_eq!(player.last_wall_dir, Some(-1));
}
