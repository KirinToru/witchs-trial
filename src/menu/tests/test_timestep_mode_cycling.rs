use crate::menu::TimestepMode;

//------------[Verify Timestep Mode Cycling]-------------------
#[test]
fn test_timestep_mode_cycling() {
    let mode = TimestepMode::Dynamic;
    assert_eq!(mode.next(), TimestepMode::Hz30);
    assert_eq!(mode.next().next(), TimestepMode::Hz60);
    assert_eq!(mode.hz(), None);
    assert_eq!(TimestepMode::Hz60.hz(), Some(60.0));
    assert_eq!(TimestepMode::Hz300.hz(), Some(300.0));
    assert_eq!(TimestepMode::Hz60.prev(), TimestepMode::Hz30);
    assert_eq!(TimestepMode::Hz30.prev(), TimestepMode::Dynamic);
}

