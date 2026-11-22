use glam::Vec2;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::InputState;
use crate::physics::{AABB, PushableBox, PUSH_SPEED_MULT_BEAST, PUSH_SPEED_MULT_WITCH};
use crate::player::{Player, PlayerForm, BEAST_SPEED_MULT};

//------------[Verify Pushing Speed For Witch And Beast And Wall Blocking]-------------------
#[test]
fn test_box_push_witch_vs_beast() {
    let mut walk_right = InputState::new();
    walk_right.on_key_event(PhysicalKey::Code(KeyCode::KeyD), winit::event::ElementState::Pressed);

    // 1. Witch pushing crate
    let mut witch = Player::new_headless(Vec2::new(82.0, 100.0));
    let mut witch_crates = vec![PushableBox::new(Vec2::new(100.0, 100.0))];
    witch.vel.x = 250.0;

    let dt = 0.016;
    witch.update(dt, &walk_right, &[], &[], &[], &[], &[], &mut witch_crates, &mut []);

    let expected_witch_push_speed = 250.0 * PUSH_SPEED_MULT_WITCH;
    assert!((witch_crates[0].vel.x - expected_witch_push_speed).abs() < 1e-2);
    assert!((witch.vel.x - expected_witch_push_speed).abs() < 1e-2);
    assert!(witch_crates[0].pos.x > 100.0);

    // 2. Beast pushing crate
    let mut beast = Player::new_headless(Vec2::new(82.0, 100.0));
    beast.form = PlayerForm::Beast;
    let mut beast_crates = vec![PushableBox::new(Vec2::new(100.0, 100.0))];
    beast.vel.x = 250.0 * BEAST_SPEED_MULT;

    beast.update(dt, &walk_right, &[], &[], &[], &[], &[], &mut beast_crates, &mut []);

    let expected_beast_push_speed = 250.0 * BEAST_SPEED_MULT * PUSH_SPEED_MULT_BEAST;
    assert!((beast_crates[0].vel.x - expected_beast_push_speed).abs() < 1e-2);
    assert!((beast.vel.x - expected_beast_push_speed).abs() < 1e-2);
    assert!(beast_crates[0].pos.x > 100.0);

    // 3. Pushing crate directly against a solid wall stops both cleanly
    let wall = vec![AABB::from_pos_size(Vec2::new(132.0, 90.0), Vec2::new(32.0, 50.0))];
    let mut blocked_player = Player::new_headless(Vec2::new(82.0, 100.0));
    let mut blocked_crates = vec![PushableBox::new(Vec2::new(100.0, 100.0))];
    blocked_player.vel.x = 250.0;

    blocked_player.update(dt, &walk_right, &wall, &[], &[], &[], &[], &mut blocked_crates, &mut []);

    // Crate must not penetrate wall (max x is 132.0 - 32.0 = 100.0)
    assert!(blocked_crates[0].pos.x <= 100.0 + 1e-3);
    assert_eq!(blocked_crates[0].vel.x, 0.0);
    assert_eq!(blocked_player.vel.x, 0.0);
}
