use glam::Vec2;
use winit::event::ElementState;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::{InputState, SfxEvent};
use crate::physics::{AABB, PushableBox};
use crate::player::Player;

//------------[Verify Dashing Into Wall-Blocked Crate Emits CrateDash SFX Only Once]-------------------
#[test]
fn test_crate_dash_single_sfx_near_wall() {
    let floor = vec![AABB::from_pos_size(Vec2::new(0.0, 132.0), Vec2::new(220.0, 32.0))];
    // Solid wall directly behind the crate right edge (100 + 32 = 132)
    let wall = vec![AABB::from_pos_size(Vec2::new(132.0, 60.0), Vec2::new(32.0, 100.0))];
    let mut solids = floor;
    solids.extend(wall);

    let mut crates = vec![PushableBox::new(Vec2::new(100.0, 100.0))];
    let mut player = Player::new_headless(Vec2::new(70.0, 106.0));
    player.grounded = true;
    player.facing = 1.0;

    // Trigger dash on first tick
    let mut dash_input = InputState::new();
    dash_input.on_key_event(PhysicalKey::Code(KeyCode::ShiftLeft), ElementState::Pressed);

    let dt = 1.0 / 60.0;
    PushableBox::update_all(&mut crates, dt, &solids, &[], &[], &[], &[], 500.0);
    player.update(dt, &dash_input, &solids, &[], &[], &[], &[], &mut crates, &mut []);

    // Release keys and simulate full dash duration + sub-steps into blocked crate
    let hold_right = {
        let mut inp = InputState::new();
        inp.on_key_event(PhysicalKey::Code(KeyCode::KeyD), ElementState::Pressed);
        inp
    };

    for _ in 0..40 {
        PushableBox::update_all(&mut crates, dt, &solids, &[], &[], &[], &[], 500.0);
        player.update(dt, &hold_right, &solids, &[], &[], &[], &[], &mut crates, &mut []);
    }

    let crate_dash_count = player
        .pending_sfx
        .iter()
        .filter(|&&e| e == SfxEvent::CrateDash)
        .count();
    assert_eq!(crate_dash_count, 1);
    assert!(crates[0].pos.x <= 100.0 + 1e-3);
}
