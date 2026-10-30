use crate::physics::AABB;
use glam::Vec2;

//------------[Verify Wall Sliding Player Does Not Accidentally Trigger Spikes Above Wall]-------------------
#[test]
fn test_spike_wall_slide_safety() {
    // Wall tile at x=100, y=100 (size 32x32)
    // Spikes on top at tile x=100, y=68, hitbox inset: pos + (3.0, 16.0), size (26.0, 16.0) -> x: 103..129, y: 84..100
    let spike_box = AABB::from_pos_size(Vec2::new(103.0, 84.0), Vec2::new(26.0, 16.0));

    // Player sliding along wall at x=80..100, overlapping in Y dimension (e.g. y=86.0)
    let player_box = AABB::from_pos_size(Vec2::new(80.0, 86.0), Vec2::new(20.0, 32.0));

    // Player touches wall edge at x=100.0, but spike is inset to x=103.0, preventing false death
    assert!(!player_box.overlaps(&spike_box));
}

