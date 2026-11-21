use glam::Vec2;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::InputState;
use crate::player::Player;

//------------[Verify Auto Bhop Mode Continuously Refreshes Jump Buffer When Space Is Held]-------------------
#[test]
fn test_auto_bhop_continuous_jump_buffer() {
    let mut player = Player::new_headless(Vec2::new(100.0, 100.0));
    player.auto_bhop = true;

    let mut input = InputState::new();
    input.on_key_event(PhysicalKey::Code(KeyCode::Space), winit::event::ElementState::Pressed);
    // Clear single-frame edge triggers so key is purely in 'held/pressed' state
    input.clear_frame_edges();

    player.update(1.0 / 60.0, &input, &[], &[], &[], &mut []);

    // Holding Space with auto-bhop enabled must refresh jump buffer to 0.16s
    assert_eq!(player.jump_buffer_timer, 0.16);
}
