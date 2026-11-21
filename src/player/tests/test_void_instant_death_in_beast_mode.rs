use glam::Vec2;
use crate::physics::AABB;
use crate::player::{Player, PlayerForm};

//------------[Verify Void Kills Player Instantly In Beast Mode Even During IFrames]-------------------
#[test]
fn test_void_instant_death_in_beast_mode() {
    let mut player = Player::new_headless(Vec2::new(100.0, 100.0));
    player.form = PlayerForm::Beast;
    player.invuln_timer = 2.0;

    // Void collider overlapping player hurtbox
    let void_tiles = vec![AABB::from_pos_size(Vec2::new(102.0, 102.0), Vec2::new(20.0, 20.0))];

    // Sub-stepped move and collide with void hazard
    player.move_and_collide(0.016, false, &[], &[], &void_tiles, &[]);

    // Void kills instantly regardless of Beast mode or active i-frames
    assert!(player.dead);
}
