use glam::Vec2;
use crate::physics::AABB;
use crate::player::Player;

//------------[Verify Wall Sliding Player Does Not Accidentally Trigger Spikes Above Wall]-------------------
#[test]
fn test_spike_wall_slide_safety() {
    // Wall at x = 100.0, player sliding along wall at x = 82.0 (right edge at 82.0 + 18.0 = 100.0)
    let player = Player::new_headless(Vec2::new(82.0, 86.0));
    assert_eq!(player.size, Vec2::new(18.0, 26.0));

    // Spike on top of wall is inset to x = 103.0
    let spikes = vec![AABB::from_pos_size(Vec2::new(103.0, 84.0), Vec2::new(26.0, 16.0))];

    // Touching wall edge at x = 100 does not overlap inset spike at x = 103
    assert!(!player.check_hazard_collision(&spikes));
}
