use glam::Vec2;
use winit::event::ElementState;
use winit::keyboard::{KeyCode, PhysicalKey};
use crate::engine::InputState;
use crate::physics::{AABB, PushableBox};
use crate::player::Player;

//------------[Simulate walking into crate while jumping at fixed timestep]-------------------
fn run_push_and_jump_scenario(solids: &[AABB], crate_x: f32, start_x: f32, dt: f32) {
    let mut crates = vec![PushableBox::new(Vec2::new(crate_x, 168.0))];
    let mut player = Player::new_headless(Vec2::new(start_x, 174.0));
    player.grounded = true;
    player.facing = 1.0;

    let mut input = InputState::new();
    input.on_key_event(PhysicalKey::Code(KeyCode::KeyD), ElementState::Pressed);

    let total_ticks = ((2.0 / dt).round() as usize).max(120);
    let jump_period = ((0.15 / dt).round() as usize).max(4);
    let push_warmup = ((0.08 / dt).round() as usize).max(4);
    let mut jumped = false;

    for tick in 0..total_ticks {
        // Push crate into fractional positions first, then pulse jump while holding right
        if tick >= push_warmup {
            let phase = (tick - push_warmup) % jump_period;
            if phase == 0 {
                input.on_key_event(PhysicalKey::Code(KeyCode::Space), ElementState::Pressed);
            } else if phase == 1 {
                input.on_key_event(PhysicalKey::Code(KeyCode::Space), ElementState::Released);
            }
        }

        PushableBox::update_all(&mut crates, dt, solids, &[], &[], &[], &[], 500.0);
        player.update(dt, &input, solids, &[], &[], &[], &[], &mut crates, &mut []);
        input.clear_frame_edges();

        if player.vel.y < -10.0 {
            jumped = true;
        }

        assert!(
            !player.dead,
            "Player died unexpectedly at tick {tick} (dt={dt}, pos={:?})",
            player.pos
        );
        assert!(
            player.pos.x > 100.0,
            "Player teleported left to {:?} at tick {tick} (dt={dt})",
            player.pos
        );
        assert!(
            player.pos.y + player.size.y <= 200.0 + 0.01,
            "Player clipped below floor to {:?} at tick {tick} (dt={dt})",
            player.pos
        );
    }

    assert!(jumped, "Expected player to jump at least once (dt={dt})");
}

//------------[Verify Walking Into Box While Jumping Never Teleports Player]-------------------
#[test]
fn test_box_push_and_jump_no_teleport() {
    let floor = vec![AABB::from_pos_size(
        Vec2::new(0.0, 200.0),
        Vec2::new(2000.0, 32.0),
    )];

    let mut blocked_solids = floor.clone();
    // Solid wall right behind the crate so push movement stops against it
    blocked_solids.push(AABB::from_pos_size(
        Vec2::new(332.0, 100.0),
        Vec2::new(32.0, 132.0),
    ));

    for &dt in &[1.0 / 60.0, 1.0 / 240.0] {
        // Free-sliding crate at standard position and near power-of-2 boundary (256.0)
        run_push_and_jump_scenario(&floor, 300.0, 260.0, dt);
        run_push_and_jump_scenario(&floor, 260.0, 242.0, dt);
        // Crate blocked directly by a solid wall
        run_push_and_jump_scenario(&blocked_solids, 300.0, 260.0, dt);
        run_push_and_jump_scenario(&blocked_solids, 300.0, 282.0, dt);
    }
}
