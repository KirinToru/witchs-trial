use glam::Vec2;
use crate::engine::InputState;
use crate::player::Player;

//------------[Verify Air Horizontal Speed Is Strictly Clamped to 300 Units in Engine Loop]-------------------
#[test]
fn test_air_speed_limit() {
    let mut player = Player::new_headless(Vec2::new(100.0, 100.0));
    player.grounded = false;
    // Set excessive initial air speed
    player.vel.x = 500.0;

    let input = InputState::new();
    player.update(1.0 / 60.0, &input, &[], &[], &[], &[], &[], &mut [], &mut []);

    // Air speed clamp must restrict horizontal velocity to 300 px/s
    assert!(player.vel.x <= 300.0);

    // Test negative air speed clamp
    player.vel.x = -650.0;
    player.update(1.0 / 60.0, &input, &[], &[], &[], &[], &[], &mut [], &mut []);
    assert!(player.vel.x >= -300.0);
}
