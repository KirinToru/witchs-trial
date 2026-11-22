use glam::Vec2;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::InputState;
use crate::player::Player;

//------------[Verify Holding Space With Auto Bhop In Air Does Not Trigger Wall Jump]-------------------
#[test]
fn test_auto_bhop_no_accidental_wall_jump() {
    let mut player = Player::new_headless(Vec2::new(100.0, 100.0));
    player.auto_bhop = true;
    player.grounded = false;

    let mut input = InputState::new();
    // Space is held in air (e.g. from jump holding) without fresh edge
    input.on_key_event(PhysicalKey::Code(KeyCode::Space), winit::event::ElementState::Pressed);
    input.clear_frame_edges();

    // Player touches right wall in mid-air
    player.walled_right = true;
    player.vel = Vec2::new(100.0, -200.0);

    // Advance simulation tick
    player.update(0.02, &input, &[], &[], &[], &[], &[], &mut [], &mut []);

    // Wall jump must not trigger because Space is only held, not freshly pressed
    assert_eq!(player.jumps_count, 0);
    assert_eq!(player.last_wall_dir, None);
    assert_eq!(player.wall_jump_buffer_timer, 0.0);
}
