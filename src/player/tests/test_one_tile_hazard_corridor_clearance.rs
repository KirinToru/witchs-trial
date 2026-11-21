use glam::Vec2;
use crate::physics::AABB;
use crate::player::Player;

//------------[Verify Player Fits and Passes Safely Through 1-Tile Gap Under Ceiling Void]-------------------
#[test]
fn test_one_tile_hazard_corridor_clearance() {
    // Ceiling void tile at y: 2..30 (with 2px inset)
    let void_hazard = vec![AABB::from_pos_size(Vec2::new(2.0, 2.0), Vec2::new(96.0, 28.0))];
    let floor_top_y = 64.0;

    // Player with 18x26 size standing on floor at pos.y = 64.0 - 26.0 = 38.0
    let mut player = Player::new_headless(Vec2::new(30.0, floor_top_y - 26.0));
    assert_eq!(player.size, Vec2::new(18.0, 26.0));
    assert_eq!(player.aabb().min.y, 38.0);
    assert_eq!(player.aabb().max.y, 64.0);

    // Standing under ceiling void does not overlap hazard
    assert!(!player.check_hazard_collision(&void_hazard));

    // Jumping into ceiling void triggers death
    player.pos.y = 25.0;
    assert!(player.check_hazard_collision(&void_hazard));
}
