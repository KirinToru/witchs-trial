use glam::Vec2;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::InputState;
use crate::physics::AABB;
use crate::player::{Player, PlayerForm, BEAST_SPEED_MULT, ICE_SPEED_MULT};

//------------[Verify Ground Speed Limit On Ice For Witch And Beast Forms]-------------------
#[test]
fn test_ice_speed_limit() {
    let ice_surface = vec![AABB::from_pos_size(Vec2::new(0.0, 100.0), Vec2::new(2000.0, 32.0))];

    // 1. Witch on ice: max ground speed should reach 250.0 * 1.35 = 337.5 px/s
    let mut witch = Player::new_headless(Vec2::new(50.0, 100.0 - 26.0));
    witch.grounded = true;
    let mut input = InputState::new();
    input.on_key_event(PhysicalKey::Code(KeyCode::KeyD), winit::event::ElementState::Pressed);

    // Accelerate over multiple ticks on solid ice surface
    for _ in 0..60 {
        witch.update(0.05, &input, &ice_surface, &[], &[], &[], &ice_surface, &mut []);
    }

    let expected_witch_max = 250.0 * ICE_SPEED_MULT;
    assert!((witch.vel.x - expected_witch_max).abs() < 0.1);
    assert!(witch.on_ice);

    // 2. Beast on ice: max ground speed should reach 250.0 * 1.40 * 1.35 = 472.5 px/s
    let mut beast = Player::new_headless(Vec2::new(50.0, 100.0 - 26.0));
    beast.form = PlayerForm::Beast;
    beast.grounded = true;

    for _ in 0..60 {
        beast.update(0.05, &input, &ice_surface, &[], &[], &[], &ice_surface, &mut []);
    }

    let expected_beast_max = 250.0 * BEAST_SPEED_MULT * ICE_SPEED_MULT;
    assert!((beast.vel.x - expected_beast_max).abs() < 0.1);
    assert!(beast.on_ice);
}
