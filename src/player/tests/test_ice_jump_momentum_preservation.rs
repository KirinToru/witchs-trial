use glam::Vec2;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::InputState;
use crate::physics::AABB;
use crate::player::{Player, PlayerForm, BEAST_SPEED_MULT, ICE_SPEED_MULT};

//------------[Verify Jump Momentum Preservation From Ice And Cancellation Conditions]-------------------
#[test]
fn test_ice_jump_momentum_preservation() {
    let ice_surface = vec![AABB::from_pos_size(Vec2::new(0.0, 100.0), Vec2::new(2000.0, 32.0))];

    // 1. Witch jump from ice: air speed must NOT clamp to 300 px/s in first air tick
    let mut witch = Player::new_headless(Vec2::new(50.0, 100.0 - 26.0));
    witch.grounded = true;
    let mut run_input = InputState::new();
    run_input.on_key_event(PhysicalKey::Code(KeyCode::KeyD), winit::event::ElementState::Pressed);

    // Accelerate to top ice speed (337.5)
    for _ in 0..40 {
        witch.update(0.05, &run_input, &ice_surface, &[], &[], &[], &ice_surface, &mut [], &mut []);
    }
    assert!((witch.vel.x - 250.0 * ICE_SPEED_MULT).abs() < 0.1);
    assert!(witch.on_ice);

    // Jump from ice
    let mut jump_input = InputState::new();
    jump_input.on_key_event(PhysicalKey::Code(KeyCode::KeyD), winit::event::ElementState::Pressed);
    jump_input.on_key_event(PhysicalKey::Code(KeyCode::Space), winit::event::ElementState::Pressed);

    witch.update(0.01, &jump_input, &ice_surface, &[], &[], &[], &ice_surface, &mut [], &mut []);
    assert!(witch.ice_jump_active);
    assert!(!witch.grounded);
    // Crucial check: horizontal velocity must not be clamped down to 300.0 px/s
    assert!(witch.vel.x > 330.0);

    // 2. Beast jump from ice: air speed must NOT clamp to 420 px/s in first air tick
    let mut beast = Player::new_headless(Vec2::new(50.0, 100.0 - 26.0));
    beast.form = PlayerForm::Beast;
    beast.grounded = true;

    for _ in 0..40 {
        beast.update(0.05, &run_input, &ice_surface, &[], &[], &[], &ice_surface, &mut [], &mut []);
    }
    assert!((beast.vel.x - 250.0 * BEAST_SPEED_MULT * ICE_SPEED_MULT).abs() < 0.1);

    beast.update(0.01, &jump_input, &ice_surface, &[], &[], &[], &ice_surface, &mut [], &mut []);
    assert!(beast.ice_jump_active);
    assert!(!beast.grounded);
    // Crucial check: horizontal velocity must not be clamped down to 420.0 px/s (250 * 1.4 * 1.35 = 472.5)
    assert!(beast.vel.x > 460.0);

    // 3. Reversing steering in mid-air cancels ice_jump_active and clamps speed to 300.0 px/s
    let mut turn_input = InputState::new();
    turn_input.on_key_event(PhysicalKey::Code(KeyCode::KeyA), winit::event::ElementState::Pressed);
    witch.update(0.01, &turn_input, &[], &[], &[], &[], &ice_surface, &mut [], &mut []);
    assert!(!witch.ice_jump_active);
    assert!(witch.vel.x <= 300.0);

    // 4. Horizontal wall collision resets ice_jump_active
    let mut wall_player = Player::new_headless(Vec2::new(50.0, 50.0));
    wall_player.ice_jump_active = true;
    wall_player.grounded = false;
    wall_player.vel = Vec2::new(320.0, 0.0);
    // Solid wall directly in path
    let solid_wall = vec![AABB::from_pos_size(Vec2::new(52.0, 40.0), Vec2::new(32.0, 64.0))];
    let no_input = InputState::new();
    wall_player.update(0.02, &no_input, &solid_wall, &[], &[], &[], &[], &mut [], &mut []);
    assert_eq!(wall_player.vel.x, 0.0);
    assert!(!wall_player.ice_jump_active);

    // 5. Landing on ground resets ice_jump_active
    let mut landing_player = Player::new_headless(Vec2::new(50.0, 39.0));
    landing_player.ice_jump_active = true;
    landing_player.grounded = false;
    landing_player.vel = Vec2::new(320.0, 100.0);
    let floor = vec![AABB::from_pos_size(Vec2::new(0.0, 70.0), Vec2::new(200.0, 32.0))];
    landing_player.update(0.05, &no_input, &floor, &[], &[], &[], &[], &mut [], &mut []);
    assert!(landing_player.grounded);
    assert!(!landing_player.ice_jump_active);
}
