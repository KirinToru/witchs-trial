use glam::Vec2;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::InputState;
use crate::player::Player;

//------------[Verify Alternating Wall Jumps Left Right Left Succeed in Air]-------------------
#[test]
fn test_wall_jump_alternating_walls() {
    let mut player = Player::new_headless(Vec2::new(100.0, 100.0));
    player.grounded = false;

    let mut input = InputState::new();

    // 1. Jump off left wall
    player.walled_left = true;
    player.walled_right = false;
    input.on_key_event(PhysicalKey::Code(KeyCode::Space), winit::event::ElementState::Pressed);
    player.update(0.01, &input, &[], &[], &[], &[], &[], &mut []);

    assert_eq!(player.jumps_count, 1);
    assert_eq!(player.last_wall_dir, Some(-1));
    assert_eq!(player.vel.x, 250.0);
    assert!(player.vel.y < -400.0);

    // 2. Touch right wall in mid-air and jump off it
    player.grounded = false;
    player.walled_left = false;
    player.walled_right = true;
    input.clear_frame_edges();
    input.on_key_event(PhysicalKey::Code(KeyCode::Space), winit::event::ElementState::Released);
    input.on_key_event(PhysicalKey::Code(KeyCode::Space), winit::event::ElementState::Pressed);
    player.update(0.01, &input, &[], &[], &[], &[], &[], &mut []);

    assert_eq!(player.jumps_count, 2);
    assert_eq!(player.last_wall_dir, Some(1));
    assert_eq!(player.vel.x, -250.0);
    assert!(player.vel.y < -400.0);

    // 3. Touch left wall again in mid-air and jump off it
    player.grounded = false;
    player.walled_left = true;
    player.walled_right = false;
    input.clear_frame_edges();
    input.on_key_event(PhysicalKey::Code(KeyCode::Space), winit::event::ElementState::Released);
    input.on_key_event(PhysicalKey::Code(KeyCode::Space), winit::event::ElementState::Pressed);
    player.update(0.01, &input, &[], &[], &[], &[], &[], &mut []);

    assert_eq!(player.jumps_count, 3);
    assert_eq!(player.last_wall_dir, Some(-1));
    assert_eq!(player.vel.x, 250.0);
    assert!(player.vel.y < -400.0);
}
