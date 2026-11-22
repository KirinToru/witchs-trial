use glam::Vec2;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::InputState;
use crate::player::{
    Player, PlayerForm, PlayerState, BEAST_DASH_DURATION_MULT, BEAST_DASH_SPEED_MULT,
    BEAST_JUMP_MULT, BEAST_SPEED_MULT,
};

//------------[Verify The Beast Mode Movement Jump and Dash Multipliers]-------------------
#[test]
fn test_beast_physics_multipliers() {
    let mut player = Player::new_headless(Vec2::new(100.0, 100.0));
    player.form = PlayerForm::Beast;

    // 1. Verify ground speed cap with multiplier (250 * 1.4 = 350)
    let max_ground_speed = 250.0 * BEAST_SPEED_MULT;
    assert_eq!(max_ground_speed, 350.0);

    // 2. Verify ground jump impulse with multiplier (-520 * 1.25 = -650, plus 12.0 ascending gravity)
    player.grounded = true;
    let mut input = InputState::new();
    input.on_key_event(PhysicalKey::Code(KeyCode::Space), winit::event::ElementState::Pressed);
    player.update(0.01, &input, &[], &[], &[], &[], &[], &mut [], &mut []);
    assert_eq!(player.vel.y, -520.0 * BEAST_JUMP_MULT + 12.0);

    // 3. Verify wall jump impulse with multiplier (-500 * 1.25 = -625, plus 12.0 ascending gravity)
    player.grounded = false;
    player.walled_left = true;
    player.last_wall_dir = None;
    input.clear_frame_edges();
    input.on_key_event(PhysicalKey::Code(KeyCode::Space), winit::event::ElementState::Released);
    input.on_key_event(PhysicalKey::Code(KeyCode::Space), winit::event::ElementState::Pressed);
    player.update(0.01, &input, &[], &[], &[], &[], &[], &mut [], &mut []);
    assert_eq!(player.vel.y, -500.0 * BEAST_JUMP_MULT + 12.0);

    // 4. Verify dash speed and duration multipliers
    let mut dash_player = Player::new_headless(Vec2::new(100.0, 100.0));
    dash_player.form = PlayerForm::Beast;
    dash_player.dash_freeze_timer = 0.01;
    let no_input = InputState::new();
    let dt = 0.02;
    dash_player.update(dt, &no_input, &[], &[], &[], &[], &[], &mut [], &mut []);

    assert_eq!(dash_player.state, PlayerState::Dash);
    // Initial duration 0.30s (0.20 * 1.5) minus dt (0.02) equals 0.28s
    let expected_initial_dur = 0.20 * BEAST_DASH_DURATION_MULT;
    assert!((dash_player.dash_timer - (expected_initial_dur - dt)).abs() < 1e-4);
    assert_eq!(600.0 * BEAST_DASH_SPEED_MULT, 810.0);
    assert!(dash_player.vel.x > 600.0);
}
