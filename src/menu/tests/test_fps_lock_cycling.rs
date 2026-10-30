use crate::menu::FpsLockOption;

//------------[Verify FPS Lock Options and Target Values]-------------------
#[test]
fn test_fps_lock_cycling() {
    let lock = FpsLockOption::Fps60;
    assert_eq!(lock.target_fps(), Some(60));
    assert_eq!(lock.next(), FpsLockOption::Fps90);
    assert_eq!(FpsLockOption::Unlimited.target_fps(), None);
}

