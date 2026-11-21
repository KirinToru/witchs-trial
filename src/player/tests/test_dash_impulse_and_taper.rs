use glam::Vec2;
use crate::engine::InputState;
use crate::player::{Player, PlayerState};

//------------[Verify Dash Impulse Starts at 600 and Tapers Down to 300 Units]-------------------
#[test]
fn test_dash_impulse_and_taper() {
    let mut player = Player::new_headless(Vec2::new(100.0, 100.0));
    player.state = PlayerState::Dash;
    player.dash_timer = 0.20;
    player.dash_dir = 1.0;
    player.vel.x = 600.0;

    let input = InputState::new();

    // Step halfway through dash (0.10s out of 0.20s)
    player.update(0.10, &input, &[], &[], &[], &mut []);
    assert_eq!(player.state, PlayerState::Dash);
    assert_eq!(player.vel.x, 450.0); // 300 + 300 * 0.5 = 450

    // Step to the end of dash duration
    player.update(0.10, &input, &[], &[], &[], &mut []);
    assert_eq!(player.vel.x, 300.0); // Exit dash speed
    assert!(player.is_bhopping);
}
