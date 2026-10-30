use crate::physics::AABB;
use glam::Vec2;

//------------[Verify Fatal Collision when Touching Side of Solid Wall Marked with Void Hazard]-------------------
#[test]
fn test_hazard_wall_contact_death() {
    let p_box = AABB::from_pos_size(Vec2::new(78.0, 100.0), Vec2::new(22.0, 32.0));
    let hazard_box = AABB::from_pos_size(Vec2::new(100.0, 100.0), Vec2::new(32.0, 32.0));

    // Exactly touches on right side (x = 100.0)
    assert!(!p_box.overlaps(&hazard_box));
    assert!(p_box.touches(&hazard_box, 1.0));
}

