use glam::Vec2;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::InputState;
use crate::physics::{AABB, PushableBox, PUSH_SPEED_MULT_WITCH};
use crate::player::Player;

//------------[Verify Crate Retains Velocity And Slides Further After Player Stops Pushing]-------------------
#[test]
fn test_box_push_release_slide_deceleration() {
    let mut walk_right = InputState::new();
    walk_right.on_key_event(PhysicalKey::Code(KeyCode::KeyD), winit::event::ElementState::Pressed);
    let no_input = InputState::new();

    let dt = 0.016;
    let floor = vec![AABB::from_pos_size(Vec2::new(0.0, 132.0), Vec2::new(1000.0, 32.0))];

    let mut player = Player::new_headless(Vec2::new(82.0, 106.0));
    player.grounded = true;
    player.vel.x = 250.0;

    let mut crates = vec![PushableBox::new(Vec2::new(100.0, 100.0))];
    crates[0].grounded = true;

    // 1. Push crate for 5 ticks with right input
    for _ in 0..5 {
        PushableBox::update_all(&mut crates, dt, &floor, &[], &[], &[], &[], 500.0);
        player.update(dt, &walk_right, &floor, &[], &[], &[], &[], &mut crates, &mut []);
    }

    let expected_push_speed = 250.0 * PUSH_SPEED_MULT_WITCH;
    assert!((crates[0].vel.x - expected_push_speed).abs() < 1e-2);
    assert!((player.vel.x - expected_push_speed).abs() < 1e-2);

    let pos_when_released = crates[0].pos.x;

    // 2. Release input (1 tick without input)
    PushableBox::update_all(&mut crates, dt, &floor, &[], &[], &[], &[], 500.0);
    player.update(dt, &no_input, &floor, &[], &[], &[], &[], &mut crates, &mut []);

    // Crate must retain horizontal velocity (> 0.0) and have moved forward
    assert!(crates[0].vel.x > 0.0);
    assert!(crates[0].pos.x > pos_when_released);
    // Crate should be decelerating under friction
    assert!(crates[0].vel.x < expected_push_speed);

    // 3. Simulate further ticks until crate stops under friction
    let mut prev_vel = crates[0].vel.x;
    let mut prev_pos = crates[0].pos.x;
    for _ in 0..50 {
        PushableBox::update_all(&mut crates, dt, &floor, &[], &[], &[], &[], 500.0);
        player.update(dt, &no_input, &floor, &[], &[], &[], &[], &mut crates, &mut []);

        if crates[0].vel.x > 0.0 {
            assert!(crates[0].vel.x <= prev_vel);
            assert!(crates[0].pos.x >= prev_pos);
            prev_vel = crates[0].vel.x;
            prev_pos = crates[0].pos.x;
        }
    }

    // Crate has come to a complete stop and slid forward from the release point
    assert_eq!(crates[0].vel.x, 0.0);
    assert!(crates[0].pos.x > pos_when_released + 10.0);
}
