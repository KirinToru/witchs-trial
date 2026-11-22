use glam::Vec2;
use crate::engine::InputState;
use crate::physics::{AABB, PushableBox, BOX_DASH_IMPULSE_WITCH, BOX_GROUND_FRICTION, BOX_ICE_FRICTION};
use crate::player::{Player, PlayerState};

//------------[Verify Dash Impact Imparts Impulse And Slides Further On Ice Than Ground]-------------------
#[test]
fn test_box_dash_impulse_and_slide() {
    // 1. Dash into crate imparts BOX_DASH_IMPULSE_WITCH (420 px/s)
    let mut player = Player::new_headless(Vec2::new(82.0, 100.0));
    player.state = PlayerState::Dash;
    player.dash_timer = 0.20;
    player.dash_dir = 1.0;
    player.vel.x = 600.0;

    let mut crates = vec![PushableBox::new(Vec2::new(100.0, 100.0))];
    let no_input = InputState::new();

    player.update(0.016, &no_input, &[], &[], &[], &[], &[], &mut crates, &mut []);

    assert_eq!(crates[0].vel.x, BOX_DASH_IMPULSE_WITCH);
    // Player horizontal velocity stopped at crate edge
    assert_eq!(player.vel.x, 0.0);
    assert_eq!(player.pos.x, 100.0 - player.size.x);

    // 2. Compare crate sliding friction on normal ground vs ice
    let floor = vec![AABB::from_pos_size(Vec2::new(0.0, 132.0), Vec2::new(1000.0, 32.0))];
    let ice = vec![AABB::from_pos_size(Vec2::new(0.0, 132.0), Vec2::new(1000.0, 32.0))];

    let mut normal_box = PushableBox::new(Vec2::new(100.0, 100.0));
    normal_box.grounded = true;
    normal_box.vel.x = 300.0;

    let mut ice_box = PushableBox::new(Vec2::new(100.0, 100.0));
    ice_box.grounded = true;
    ice_box.vel.x = 300.0;

    let dt = 0.1;
    normal_box.update_step(dt, &floor, &[], &[], &[], &[], &[], 500.0);
    ice_box.update_step(dt, &floor, &[], &ice, &[], &[], &[], 500.0);

    // Normal friction reduces speed by BOX_GROUND_FRICTION (900 * 0.1 = 90 px/s)
    let expected_normal_speed = 300.0 - BOX_GROUND_FRICTION * dt;
    // Ice friction reduces speed by BOX_ICE_FRICTION (120 * 0.1 = 12 px/s)
    let expected_ice_speed = 300.0 - BOX_ICE_FRICTION * dt;

    assert!((normal_box.vel.x - expected_normal_speed).abs() < 1e-3);
    assert!((ice_box.vel.x - expected_ice_speed).abs() < 1e-3);
    assert!(ice_box.vel.x > normal_box.vel.x);
}
