use glam::Vec2;
use winit::event::ElementState;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::{InputState, SfxEvent};
use crate::physics::{AABB, PushableBox};
use crate::player::Player;

//------------[Simulate walking rightward against a crate for several frames]-------------------
fn push_crate_for_frames(solids: &[AABB], crate_x: f32) -> (Player, Vec<PushableBox>) {
    let mut crates = vec![PushableBox::new(Vec2::new(crate_x, 168.0))];
    let mut player = Player::new_headless(Vec2::new(crate_x - 24.0, 174.0));
    player.grounded = true;
    player.facing = 1.0;

    let mut input = InputState::new();
    input.on_key_event(PhysicalKey::Code(KeyCode::KeyD), ElementState::Pressed);

    let dt = 1.0 / 60.0;
    for _ in 0..15 {
        PushableBox::update_all(&mut crates, dt, solids, &[], &[], &[], &[], 500.0);
        player.update(dt, &input, solids, &[], &[], &[], &[], &mut crates, &mut []);
        input.clear_frame_edges();
    }

    (player, crates)
}

//------------[Verify CratePush SFX Emits Only When Pushing Actually Moves Crate]-------------------
#[test]
fn test_crate_push_sfx_emitted_only_when_moving() {
    let floor = vec![AABB::from_pos_size(
        Vec2::new(0.0, 200.0),
        Vec2::new(640.0, 32.0),
    )];

    // 1. Free crate moves when pushed and must emit CratePush SFX
    let (free_player, free_crates) = push_crate_for_frames(&floor, 200.0);
    assert!(
        free_crates[0].pos.x > 201.0,
        "Free crate should move when pushed, got x={}",
        free_crates[0].pos.x
    );
    assert!(
        free_player.pending_sfx.contains(&SfxEvent::CratePush),
        "Pushing a moving crate should emit SfxEvent::CratePush"
    );

    // 2. Crate flush against a solid wall does not move and must not emit CratePush SFX
    let mut blocked_solids = floor;
    blocked_solids.push(AABB::from_pos_size(
        Vec2::new(232.0, 100.0),
        Vec2::new(32.0, 132.0),
    ));
    let (blocked_player, blocked_crates) = push_crate_for_frames(&blocked_solids, 200.0);
    assert!(
        blocked_crates[0].pos.x <= 200.0 + 1e-4,
        "Wall-blocked crate must stay stationary, got x={}",
        blocked_crates[0].pos.x
    );
    assert!(
        !blocked_player.pending_sfx.contains(&SfxEvent::CratePush),
        "Pushing a wall-blocked crate must not emit SfxEvent::CratePush"
    );
}
