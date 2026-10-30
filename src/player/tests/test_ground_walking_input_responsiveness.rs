use crate::player::move_towards;

//------------[Verify Grounded Player Accelerates Normally When Pressing Directional Keys]-------------------
#[test]
fn test_ground_walking_input_responsiveness() {
    let grounded = true;
    let is_bhopping = false;
    let bhop_timer = 0.0;
    let move_x = 1.0;
    let dt = 1.0 / 60.0;
    let accel = 1800.0;
    let max_ground_speed = 250.0;
    let mut vel_x = 0.0;

    if grounded {
        if bhop_timer > 0.0 && is_bhopping {
            // Skips friction during bhop
        } else if move_x != 0.0 {
            let target_vel = move_x * max_ground_speed;
            vel_x = move_towards(vel_x, target_vel, accel * dt);
        }
    }

    assert!(vel_x > 0.0);
    assert!((vel_x - 30.0).abs() < 0.001); // 1800 * (1/60) approx 30 px/s after 1 tick
}
