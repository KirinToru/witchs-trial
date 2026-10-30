//------------[Verify Wall Jump Lock Cancellation when Steering Back Into Wall]-------------------
#[test]
fn test_wall_jump_lock_cancellation() {
    let mut lock_timer = 0.15;
    let walled_left = true;
    let move_x = -1.0;

    if move_x != 0.0 && (walled_left && move_x < 0.0) {
        lock_timer = 0.0;
    }

    assert_eq!(lock_timer, 0.0);
}

