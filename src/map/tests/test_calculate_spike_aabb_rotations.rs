use crate::map::calculate_spike_aabb;
use glam::Vec2;

//------------[Verify Rotated and Flipped Spike Hitbox Alignment]-------------------
#[test]
fn test_calculate_spike_aabb_rotations() {
    let pos = Vec2::new(100.0, 100.0);

    // 1. Unrotated spikes (0 deg) -> bottom half
    let aabb_0 = calculate_spike_aabb(pos, 0.0, Vec2::new(1.0, 1.0));
    assert_eq!(aabb_0.min, Vec2::new(103.0, 116.0));
    assert_eq!(aabb_0.max, Vec2::new(129.0, 132.0));

    // 2. Ceiling spikes (180 deg) -> top half
    let aabb_180 = calculate_spike_aabb(pos, 180.0, Vec2::new(1.0, 1.0));
    assert!((aabb_180.min.x - 103.0).abs() < 0.01);
    assert!((aabb_180.min.y - 100.0).abs() < 0.01);
    assert!((aabb_180.max.x - 129.0).abs() < 0.01);
    assert!((aabb_180.max.y - 116.0).abs() < 0.01);

    // 3. Wall spikes pointing right (90 deg) -> left half
    let aabb_90 = calculate_spike_aabb(pos, 90.0, Vec2::new(1.0, 1.0));
    assert!((aabb_90.min.x - 100.0).abs() < 0.01);
    assert!((aabb_90.min.y - 103.0).abs() < 0.01);
    assert!((aabb_90.max.x - 116.0).abs() < 0.01);
    assert!((aabb_90.max.y - 129.0).abs() < 0.01);
}

