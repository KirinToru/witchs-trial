use crate::physics::AABB;
use glam::Vec2;

//------------[Verify AABB Touching and Proximity Detection with Tolerance]-------------------
#[test]
fn test_aabb_touches() {
    let box1 = AABB::from_pos_size(Vec2::new(0.0, 0.0), Vec2::new(10.0, 10.0));
    // box2 touches box1 exactly at edge x = 10.0
    let box2 = AABB::from_pos_size(Vec2::new(10.0, 0.0), Vec2::new(10.0, 10.0));
    let box3 = AABB::from_pos_size(Vec2::new(12.0, 0.0), Vec2::new(10.0, 10.0));

    assert!(!box1.overlaps(&box2));
    assert!(box1.touches(&box2, 0.5));
    assert!(!box1.touches(&box3, 0.5));
    assert!(box1.touches(&box3, 2.5));
}

