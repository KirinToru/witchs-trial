use crate::physics::AABB;
use glam::Vec2;

//------------[Verify Player Fits and Passes Safely Through 1-Tile Gap Under Ceiling Void]-------------------
#[test]
fn test_one_tile_hazard_corridor_clearance() {
    // 32px grid layout:
    // Y: 0..32 -> Ceiling Void tile (with 2px safety inset: y: 2..30)
    // Y: 32..64 -> Empty space (1 tile = 32px corridor)
    // Y: 64..96 -> Solid Floor block
    let void_hazard = AABB::from_pos_size(Vec2::new(0.0 + 2.0, 0.0 + 2.0), Vec2::new(96.0, 28.0));
    let floor_top_y = 64.0;

    // Player with 18x26 bounding box standing on floor: pos.y = 64 - 26 = 38
    let player_pos = Vec2::new(30.0, floor_top_y - 26.0);
    let player_size = Vec2::new(18.0, 26.0);
    let hurtbox_inset = Vec2::new(2.0, 2.0);

    let player_box = AABB::from_pos_size(player_pos, player_size);
    let player_hurtbox = AABB::new(player_pos + hurtbox_inset, player_pos + player_size - hurtbox_inset);

    // 1. Verify player bounding box and hurtbox fit inside the 32px vertical gap
    assert_eq!(player_box.min.y, 38.0);
    assert_eq!(player_box.max.y, 64.0);
    assert!(player_box.min.y >= 32.0); // Inside the empty gap (32..64)

    // 2. Verify player standing or walking under the ceiling void does NOT overlap the hazard
    assert!(!player_box.overlaps(&void_hazard));
    assert!(!player_hurtbox.overlaps(&void_hazard));

    // 3. Verify jumping into the ceiling void penetrates the hazard and triggers fatal collision
    let jumping_hurtbox = AABB::new(
        Vec2::new(player_pos.x, 25.0) + hurtbox_inset,
        Vec2::new(player_pos.x, 25.0) + player_size - hurtbox_inset,
    );
    assert!(jumping_hurtbox.overlaps(&void_hazard));
}

