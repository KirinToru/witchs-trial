use glam::Vec2;
use crate::physics::AABB;
use crate::player::{Player, PlayerForm, BEAST_HIT_KNOCKBACK_X, BEAST_HIT_KNOCKBACK_Y};

//------------[Verify Spike Hit In Beast Mode Reverts To Witch With Knockback And IFrames]-------------------
#[test]
fn test_beast_spike_hit_reversion_and_iframes() {
    let mut player = Player::new_headless(Vec2::new(100.0, 100.0));
    player.form = PlayerForm::Beast;
    player.facing = 1.0;
    player.vel = Vec2::new(200.0, 50.0);

    // Spike overlapping player hurtbox
    let spikes = vec![AABB::from_pos_size(Vec2::new(105.0, 105.0), Vec2::new(20.0, 20.0))];

    // First spike hit
    player.handle_spike_collision(&spikes);

    // Must revert to Witch, gain 2.0s invulnerability, survive, and get knocked back
    assert_eq!(player.form, PlayerForm::Witch);
    assert_eq!(player.invuln_timer, 2.0);
    assert!(!player.dead);
    assert_eq!(player.vel.y, BEAST_HIT_KNOCKBACK_Y);
    assert_eq!(player.vel.x, -BEAST_HIT_KNOCKBACK_X);

    // Second spike contact while invulnerable must be ignored
    player.handle_spike_collision(&spikes);
    assert!(!player.dead);
    assert_eq!(player.invuln_timer, 2.0);
}
