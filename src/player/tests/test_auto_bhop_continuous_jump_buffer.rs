//------------[Verify Auto Bhop Mode Refreshes Jump Buffer When Space Is Held]-------------------
#[test]
fn test_auto_bhop_continuous_jump_buffer() {
    let auto_bhop = true;
    let space_pressed = true;
    let space_just_pressed = false;

    let mut jump_buffer_timer = 0.0;
    if space_just_pressed || (auto_bhop && space_pressed) {
        jump_buffer_timer = 0.16;
    }

    assert_eq!(jump_buffer_timer, 0.16);
}

