use glam::Vec2;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::InputState;
use crate::physics::PushableBox;
use crate::player::Player;

//------------[Verify Standing And Jumping On Box Without Triggering Wall Probes On Box Sides]-------------------
#[test]
fn test_box_stand_and_jump_no_wall_probe() {
    // Crate at x = 100.0, y = 100.0, size 32x32 (top edge at y = 100.0)
    let mut crates = vec![PushableBox::new(Vec2::new(100.0, 100.0))];

    // Player placed directly on top of crate: y = 100.0 - 26.0 = 74.0
    let mut player = Player::new_headless(Vec2::new(106.0, 74.0));
    let no_input = InputState::new();

    // 1 tick of simulation resting on crate top
    player.update(0.016, &no_input, &[], &[], &[], &[], &[], &mut crates, &mut []);

    assert!(player.grounded);
    assert_eq!(player.vel.y, 0.0);
    assert_eq!(player.pos.y, 100.0 - player.size.y);

    // Jump from top of crate with Space
    let mut jump_input = InputState::new();
    jump_input.on_key_event(PhysicalKey::Code(KeyCode::Space), winit::event::ElementState::Pressed);
    player.update(0.016, &jump_input, &[], &[], &[], &[], &[], &mut crates, &mut []);

    // Player jumps upwards (negative vertical velocity)
    assert!(player.vel.y < 0.0);
    assert!(!player.grounded);

    // 2. Side contact must NOT trigger wall slides or wall jumps
    let mut side_player = Player::new_headless(Vec2::new(100.0 - 18.0, 104.0));
    side_player.update(0.016, &no_input, &[], &[], &[], &[], &[], &mut crates, &mut []);

    // Wall probes only check solid_tiles, so box does not set walled_right or walled_left
    assert!(!side_player.walled_left);
    assert!(!side_player.walled_right);
}
