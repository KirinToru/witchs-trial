use glam::Vec2;
use crate::physics::{PushableBox, AABB};

//------------[Verify Pushable Box Falls Under Gravity And Lands Safely On Ground And Spikes]-------------------
#[test]
fn test_box_gravity_and_landing() {
    let mut krate = PushableBox::new(Vec2::new(100.0, 50.0));
    let floor = vec![AABB::from_pos_size(Vec2::new(50.0, 100.0), Vec2::new(200.0, 32.0))];

    // 1. Box starts in air and falls
    assert!(!krate.grounded);
    krate.update_step(0.1, &floor, &[], &[], &[], &[], &[], 500.0);
    assert!(krate.vel.y > 0.0);

    // Continue falling until landing on floor (top of floor is y = 100.0)
    for _ in 0..10 {
        krate.update_step(0.05, &floor, &[], &[], &[], &[], &[], 500.0);
    }
    assert!(krate.grounded);
    assert_eq!(krate.vel.y, 0.0);
    assert_eq!(krate.pos.y, 100.0 - 32.0);

    // 2. Landing on spikes (box is immune and uses spikes as solid ground)
    let mut spike_krate = PushableBox::new(Vec2::new(200.0, 50.0));
    let spikes = vec![AABB::from_pos_size(Vec2::new(150.0, 120.0), Vec2::new(100.0, 32.0))];
    for _ in 0..10 {
        spike_krate.update_step(0.05, &[], &[], &[], &[], &spikes, &[], 500.0);
    }
    assert!(spike_krate.active);
    assert!(spike_krate.grounded);
    assert_eq!(spike_krate.vel.y, 0.0);
    assert_eq!(spike_krate.pos.y, 120.0 - 32.0);
}
