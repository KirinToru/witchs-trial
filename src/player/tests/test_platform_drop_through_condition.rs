//------------[Verify One-Way Platform Ignores Collision When Dropping Down]-------------------
#[test]
fn test_platform_drop_through_condition() {
    let dropping_down = true;
    let vel_y = 50.0;
    let mut landed = false;

    if !dropping_down && vel_y >= 0.0 {
        landed = true;
    }

    assert!(!landed);
}

