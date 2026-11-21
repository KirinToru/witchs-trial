use glam::Vec2;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::InputState;
use crate::player::Player;

//------------[Verify Auto Bhop Does Not Buffer Jump In Air But Triggers On Ground]-------------------
#[test]
fn test_auto_bhop_continuous_jump_buffer() {
    let mut player = Player::new_headless(Vec2::new(100.0, 100.0));
    player.auto_bhop = true;

    let mut input = InputState::new();
    input.on_key_event(PhysicalKey::Code(KeyCode::Space), winit::event::ElementState::Pressed);
    // Clear single-frame edge triggers so key is purely in 'held/pressed' state
    input.clear_frame_edges();

    // 1. While airborne, holding Space must NOT load jump buffers
    player.grounded = false;
    player.update(1.0 / 60.0, &input, &[], &[], &[], &[], &mut []);

    assert_eq!(player.jump_buffer_timer, 0.0);
    assert_eq!(player.wall_jump_buffer_timer, 0.0);
    assert_eq!(player.jumps_count, 0);

    // 2. While grounded, holding Space with auto-bhop must trigger jump
    player.grounded = true;
    player.update(1.0 / 60.0, &input, &[], &[], &[], &[], &mut []);

    assert_eq!(player.jumps_count, 1);
    assert!(player.vel.y < 0.0);
    assert_eq!(player.jump_buffer_timer, 0.0);
    assert_eq!(player.wall_jump_buffer_timer, 0.0);
}
