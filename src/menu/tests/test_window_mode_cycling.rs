use crate::menu::WindowDisplayMode;

//------------[Verify Window Mode Cycling Operations]-------------------
#[test]
fn test_window_mode_cycling() {
    let mode = WindowDisplayMode::Windowed;
    assert_eq!(mode.next(), WindowDisplayMode::Borderless);
    assert_eq!(mode.next().next(), WindowDisplayMode::Exclusive);
    assert_eq!(mode.next().next().next(), WindowDisplayMode::Windowed);
    assert_eq!(mode.prev(), WindowDisplayMode::Exclusive);
}

