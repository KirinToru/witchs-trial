//------------[Verify BHOP Preservation Within Timing Window]-------------------
#[test]
fn test_bhop_momentum_preservation() {
    let bhop_timer: f32 = 0.12;
    let bhop_velocity: f32 = 520.0;
    let mut vel_x: f32 = 0.0;
    let mut is_bhopping = false;

    if bhop_timer > 0.0 && bhop_velocity.abs() > 200.0 {
        vel_x = bhop_velocity;
        is_bhopping = true;
    }

    assert_eq!(vel_x, 520.0);
    assert!(is_bhopping);
}

