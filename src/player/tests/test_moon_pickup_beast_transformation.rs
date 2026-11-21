use glam::Vec2;
use crate::engine::InputState;
use crate::map::{InteractiveKind, InteractiveTile};
use crate::player::{Player, PlayerForm};

//------------[Verify Collecting Moon Item Activates It and Triggers Beast Form]-------------------
#[test]
fn test_moon_pickup_beast_transformation() {
    let mut player = Player::new_headless(Vec2::new(100.0, 100.0));
    assert_eq!(player.form, PlayerForm::Witch);

    // Create an unactivated Moon item at player position
    let mut interactives = vec![InteractiveTile {
        pos: Vec2::new(100.0, 100.0),
        size: Vec2::new(32.0, 32.0),
        kind: InteractiveKind::Moon,
        activated: false,
    }];

    let input = InputState::new();
    player.update(1.0 / 60.0, &input, &[], &[], &[], &[], &[], &mut interactives);

    // Moon item must be activated and player transformed into Beast
    assert!(interactives[0].activated);
    assert_eq!(player.form, PlayerForm::Beast);
}
