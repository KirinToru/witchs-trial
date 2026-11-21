use glam::Vec2;
use crate::physics::AABB;
use crate::player::Player;

//------------[Verify Dash Does Not Tunnel Through Thin Wall At 30 Hz Due To Sub-Stepping]-------------------
#[test]
fn test_dash_substepping_tunneling_prevention() {
    // Player at x = 80.0 with width 18.0 (right edge at 98.0)
    let mut player = Player::new_headless(Vec2::new(80.0, 100.0));
    // High dash speed of 600 px/s
    player.vel.x = 600.0;

    // Thin wall 8px wide located between x = 105.0 and 113.0
    let thin_wall = vec![AABB::from_pos_size(Vec2::new(105.0, 90.0), Vec2::new(8.0, 40.0))];

    // At 30 Hz timestep (dt = 1/30 ≈ 0.0333s), single displacement would be 20px (from 80 to 100, edge 118)
    // Without sub-stepping, player would tunnel completely past the 8px wall.
    // With sub-stepping (threshold 8.0 px), the step is split into 3 sub-steps and collision is detected.
    let dt = 1.0 / 30.0;
    player.move_and_collide(dt, false, &thin_wall, &[], &[], &[]);

    // Player must stop in front of the wall (x = 105.0 - 18.0 = 87.0) with zero velocity
    assert_eq!(player.pos.x, 105.0 - player.size.x);
    assert_eq!(player.vel.x, 0.0);
}
