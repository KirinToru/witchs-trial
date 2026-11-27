use glam::Vec2;
use winit::event::ElementState;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::{InputState, SfxEvent};
use crate::map::{InteractiveKind, InteractiveTile};
use crate::physics::AABB;
use crate::player::Player;

//------------[Verify Player Emits Pending SFX Events For Jump Dash And Beast Transform]-------------------
#[test]
fn test_player_audio_events_emitted() {
    let floor = vec![AABB::from_pos_size(Vec2::new(0.0, 200.0), Vec2::new(400.0, 32.0))];
    let mut player = Player::new_headless(Vec2::new(100.0, 200.0 - 26.0));
    player.grounded = true;

    // 1. Collect Moon item and jump simultaneously -> emits BeastTransform and Jump
    let mut interactives = vec![InteractiveTile {
        pos: Vec2::new(96.0, 174.0),
        size: Vec2::new(32.0, 32.0),
        kind: InteractiveKind::Moon,
        activated: false,
    }];

    let mut jump_input = InputState::new();
    jump_input.on_key_event(PhysicalKey::Code(KeyCode::Space), ElementState::Pressed);

    player.update(
        1.0 / 60.0,
        &jump_input,
        &floor,
        &[],
        &[],
        &[],
        &[],
        &mut [],
        &mut interactives,
    );

    assert!(player.pending_sfx.contains(&SfxEvent::BeastTransform));
    assert!(player.pending_sfx.contains(&SfxEvent::Jump));
    player.pending_sfx.clear();

    // 2. Trigger Dash -> emits Dash SFX event
    let mut dash_input = InputState::new();
    dash_input.on_key_event(PhysicalKey::Code(KeyCode::ShiftLeft), ElementState::Pressed);

    player.update(
        1.0 / 60.0,
        &dash_input,
        &floor,
        &[],
        &[],
        &[],
        &[],
        &mut [],
        &mut interactives,
    );

    assert!(player.pending_sfx.contains(&SfxEvent::Dash));
}
