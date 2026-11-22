use glam::Vec2;
use crate::physics::{PushableBox, AABB};

//------------[Verify Pushable Box Deactivates In Void Or Bottom Pit And Resets Correctly]-------------------
#[test]
fn test_box_void_death_and_reset() {
    let spawn = Vec2::new(100.0, 100.0);
    let mut krate = PushableBox::new(spawn);
    assert!(krate.active);

    // 1. Deactivation when falling into void collider
    let void_tiles = vec![AABB::from_pos_size(Vec2::new(80.0, 120.0), Vec2::new(60.0, 32.0))];
    // Move box down into void
    krate.pos.y = 125.0;
    krate.update_step(0.02, &[], &[], &[], &void_tiles, &[], &[], 500.0);
    assert!(!krate.active);

    // 2. Reset restores spawn position and active state
    krate.reset();
    assert!(krate.active);
    assert_eq!(krate.pos, spawn);
    assert_eq!(krate.vel, Vec2::ZERO);

    // 3. Deactivation when falling below map bottom
    let map_bottom = 300.0;
    krate.pos.y = 350.0;
    krate.update_step(0.02, &[], &[], &[], &[], &[], &[], map_bottom);
    assert!(!krate.active);

    // Reset restores again
    krate.reset();
    assert!(krate.active);
    assert_eq!(krate.pos, spawn);
}
