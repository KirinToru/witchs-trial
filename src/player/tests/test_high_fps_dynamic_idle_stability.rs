use glam::Vec2;
use crate::engine::InputState;
use crate::physics::AABB;
use crate::player::{Player, PlayerState};

//------------[Verify High FPS Dynamic Timestep Keeps Player Grounded and Idle Animation Stable]-------------------
#[test]
fn test_high_fps_dynamic_idle_stability() {
    // High Y coordinate (4200.0) exercises f32 precision limits where tiny dt steps round to zero
    let floor_y = 4200.0;
    let floor = vec![AABB::from_pos_size(Vec2::new(0.0, floor_y), Vec2::new(400.0, 32.0))];

    let mut player = Player::new_headless(Vec2::new(100.0, floor_y - 26.0));
    player.grounded = true;

    let input = InputState::new();
    let dt = 1.0 / 3000.0;
    let mut frame_changes = 0;
    let mut prev_frame = player.anim_frame;

    // Simulate 1.0 second at 3000 FPS in Dynamic mode
    for _ in 0..3000 {
        player.update(dt, &input, &floor, &[], &[], &[], &[], &mut [], &mut []);

        // Ground contact and Idle state must stay stable without flickering to Fall
        assert!(player.grounded);
        assert_eq!(player.state, PlayerState::Idle);
        assert_eq!(player.vel.y, 0.0);

        if player.anim_frame != prev_frame {
            frame_changes += 1;
            prev_frame = player.anim_frame;
        }
    }

    // Idle frame duration is 0.30s, so 1.0s must produce 3 frame advances
    let (_, _, idle_frame_duration) = PlayerState::Idle.anim_params();
    let expected_changes = (1.0 / idle_frame_duration).floor() as usize;
    assert_eq!(frame_changes, expected_changes);
}
