use glam::Vec2;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::InputState;
use crate::player::Player;

//------------[Verify Wall Jump Buffer Window And Clearance On Ground Jump]-------------------
#[test]
fn test_wall_jump_short_buffer_cleared_on_ground_jump() {
    // 1. Fresh press within 0.08s window triggers wall jump when touching wall
    let mut player = Player::new_headless(Vec2::new(100.0, 100.0));
    player.grounded = false;
    player.walled_left = false;

    let mut input = InputState::new();
    input.on_key_event(PhysicalKey::Code(KeyCode::Space), winit::event::ElementState::Pressed);

    // Initial tick: fresh press loads buffer to 0.08s
    player.update(0.02, &input, &[], &[], &[], &[], &[], &mut []);
    assert_eq!(player.jumps_count, 0);
    assert_eq!(player.wall_jump_buffer_timer, 0.08);

    // Second tick: advancing time without fresh press decrements buffer (0.08 - 0.03 = 0.05)
    input.clear_frame_edges();
    player.update(0.03, &input, &[], &[], &[], &[], &[], &mut []);
    assert_eq!(player.jumps_count, 0);
    assert!((player.wall_jump_buffer_timer - 0.05).abs() < 1e-4);

    // Third tick: player touches wall within remaining buffer window
    player.walled_left = true;
    player.update(0.02, &input, &[], &[], &[], &[], &[], &mut []);

    // Wall jump successfully executed from buffer
    assert_eq!(player.jumps_count, 1);
    assert_eq!(player.last_wall_dir, Some(-1));
    assert_eq!(player.wall_jump_buffer_timer, 0.0);

    // 2. Jumping from the ground next to a wall zeroes the buffer and prevents immediate wall jump
    let mut ground_player = Player::new_headless(Vec2::new(100.0, 100.0));
    ground_player.grounded = true;
    ground_player.walled_left = true;

    let mut ground_input = InputState::new();
    ground_input.on_key_event(PhysicalKey::Code(KeyCode::Space), winit::event::ElementState::Pressed);

    // Execute ground jump
    ground_player.update(0.01, &ground_input, &[], &[], &[], &[], &[], &mut []);
    assert_eq!(ground_player.jumps_count, 1);
    // Ground jump must clear both buffers
    assert_eq!(ground_player.jump_buffer_timer, 0.0);
    assert_eq!(ground_player.wall_jump_buffer_timer, 0.0);
    assert_eq!(ground_player.last_wall_dir, None);

    // Next tick: player is airborne next to wall, Space remains held
    ground_input.clear_frame_edges();
    ground_player.update(0.01, &ground_input, &[], &[], &[], &[], &[], &mut []);

    // Wall jump must not trigger immediately after ground jump
    assert_eq!(ground_player.jumps_count, 1);
    assert_eq!(ground_player.last_wall_dir, None);
}
