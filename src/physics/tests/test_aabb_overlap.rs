use crate::physics::AABB;
use glam::Vec2;

//------------[Verify AABB Intersection Detection Logic]-------------------
#[test]
fn test_aabb_overlap() {
    let box1 = AABB::from_pos_size(Vec2::new(0.0, 0.0), Vec2::new(10.0, 10.0));
    let box2 = AABB::from_pos_size(Vec2::new(5.0, 5.0), Vec2::new(10.0, 10.0));
    let box3 = AABB::from_pos_size(Vec2::new(20.0, 20.0), Vec2::new(10.0, 10.0));

    assert!(box1.overlaps(&box2));
    assert!(!box1.overlaps(&box3));
}

