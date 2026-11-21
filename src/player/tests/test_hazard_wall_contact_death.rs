use glam::Vec2;
use crate::physics::AABB;
use crate::player::Player;

//------------[Verify Hazard Contact Rules with Standard Player Size and Inset Hurtbox]-------------------
#[test]
fn test_hazard_wall_contact_death() {
    let player = Player::new_headless(Vec2::new(100.0, 100.0));
    assert_eq!(player.size, Vec2::new(18.0, 26.0));

    // Touching player right edge (x = 118.0) does not overlap inset hurtbox (max x = 116.0)
    let touching_hazard = vec![AABB::from_pos_size(Vec2::new(118.0, 100.0), Vec2::new(32.0, 32.0))];
    assert!(!player.check_hazard_collision(&touching_hazard));

    // Penetrating into hurtbox triggers lethal collision
    let penetrating_hazard = vec![AABB::from_pos_size(Vec2::new(115.0, 100.0), Vec2::new(32.0, 32.0))];
    assert!(player.check_hazard_collision(&penetrating_hazard));
}
