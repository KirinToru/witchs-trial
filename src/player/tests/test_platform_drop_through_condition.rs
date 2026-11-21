use glam::Vec2;
use crate::physics::AABB;
use crate::player::Player;

//------------[Verify One-Way Platform Ignores Collision When Dropping Down with Key S]-------------------
#[test]
fn test_platform_drop_through_condition() {
    let mut player = Player::new_headless(Vec2::new(100.0, 82.0));
    // Platform at y = 106.0 (player bottom was at 75.0 + 26.0 = 101.0, now at 82.0 + 26.0 = 108.0)
    let platforms = vec![AABB::from_pos_size(Vec2::new(90.0, 106.0), Vec2::new(40.0, 8.0))];
    let prev_pos = Vec2::new(100.0, 75.0);
    player.vel.y = 50.0;

    // Dropping down ignores platform collision
    player.resolve_one_way_platforms(prev_pos, true, &platforms);
    assert!(!player.grounded);

    // Falling onto platform without dropping down snaps player to top of platform
    player.resolve_one_way_platforms(prev_pos, false, &platforms);
    assert!(player.grounded);
    assert_eq!(player.vel.y, 0.0);
    assert_eq!(player.pos.y, 106.0 - player.size.y);
}
