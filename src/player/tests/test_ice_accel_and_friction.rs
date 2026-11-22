use glam::Vec2;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::InputState;
use crate::physics::AABB;
use crate::player::{Player, ICE_ACCEL, ICE_FRICTION};

//------------[Verify Slower Acceleration And Lower Friction On Ice]-------------------
#[test]
fn test_ice_accel_and_friction() {
    let solid_floor = vec![AABB::from_pos_size(Vec2::new(0.0, 100.0), Vec2::new(500.0, 32.0))];
    let dt = 0.1;

    // 1. Acceleration test: starting from rest, 1 tick of right input on ice vs normal ground
    let mut player_ice = Player::new_headless(Vec2::new(50.0, 100.0 - 26.0));
    player_ice.grounded = true;

    let mut player_normal = Player::new_headless(Vec2::new(50.0, 100.0 - 26.0));
    player_normal.grounded = true;

    let mut walk_input = InputState::new();
    walk_input.on_key_event(PhysicalKey::Code(KeyCode::KeyD), winit::event::ElementState::Pressed);

    player_ice.update(dt, &walk_input, &solid_floor, &[], &[], &[], &solid_floor, &mut [], &mut []);
    player_normal.update(dt, &walk_input, &solid_floor, &[], &[], &[], &[], &mut [], &mut []);

    // Ice acceleration is 450.0 * dt = 45.0 px/s, normal is 1800.0 * dt = 180.0 px/s
    assert!((player_ice.vel.x - (ICE_ACCEL * dt)).abs() < 1e-3);
    assert!((player_normal.vel.x - (1800.0 * dt)).abs() < 1e-3);
    assert!(player_ice.vel.x < player_normal.vel.x);

    // 2. Friction slide test: release input with initial speed 300.0 px/s
    player_ice.vel.x = 300.0;
    player_normal.vel.x = 300.0;
    let no_input = InputState::new();

    player_ice.update(dt, &no_input, &solid_floor, &[], &[], &[], &solid_floor, &mut [], &mut []);
    player_normal.update(dt, &no_input, &solid_floor, &[], &[], &[], &[], &mut [], &mut []);

    // Ice friction reduces speed by ICE_FRICTION * dt (250 * 0.1 = 25 px/s)
    let expected_ice_speed = 300.0 - ICE_FRICTION * dt;
    // Normal friction reduces speed by 2000 * 0.1 = 200 px/s
    let expected_normal_speed = 300.0 - 2000.0 * dt;

    assert!((player_ice.vel.x - expected_ice_speed).abs() < 1e-3);
    assert!((player_normal.vel.x - expected_normal_speed).abs() < 1e-3);
    assert!(player_ice.vel.x > player_normal.vel.x);
}
