use glam::Vec2;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::InputState;
use crate::player::Player;

//------------[Verify Landing On Ground Resets Wall Jump Limit Allowing Same Wall Again]-------------------
#[test]
fn test_wall_jump_ground_reset() {
    let mut player = Player::new_headless(Vec2::new(100.0, 100.0));
    player.grounded = false;
    player.walled_left = true;

    let mut input = InputState::new();
    input.on_key_event(PhysicalKey::Code(KeyCode::Space), winit::event::ElementState::Pressed);

    // 1. Jump off left wall in air
    player.update(0.01, &input, &[], &[], &[], &[], &[], &mut [], &mut []);
    assert_eq!(player.jumps_count, 1);
    assert_eq!(player.last_wall_dir, Some(-1));

    // 2. Clear edge triggers, release Space and land on ground
    input.clear_frame_edges();
    input.on_key_event(PhysicalKey::Code(KeyCode::Space), winit::event::ElementState::Released);
    player.grounded = true;
    player.update(0.01, &input, &[], &[], &[], &[], &[], &mut [], &mut []);

    // Ground contact resets the last wall limit without performing another jump
    assert_eq!(player.jumps_count, 1);
    assert_eq!(player.last_wall_dir, None);

    // 3. Leave ground and touch the same left wall
    player.grounded = false;
    player.walled_left = true;
    player.walled_right = false;
    input.clear_frame_edges();
    input.on_key_event(PhysicalKey::Code(KeyCode::Space), winit::event::ElementState::Pressed);
    player.update(0.01, &input, &[], &[], &[], &[], &[], &mut [], &mut []);

    // Jumping off left wall succeeds again after landing
    assert_eq!(player.jumps_count, 2);
    assert_eq!(player.last_wall_dir, Some(-1));
}
