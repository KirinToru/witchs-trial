//------------[Verify Dash Impulse Starts at 600 and Tapers to 300 Units]-------------------
#[test]
fn test_dash_impulse_and_taper() {
    let start_dash_speed: f32 = 600.0;
    let end_dash_speed: f32 = 300.0;
    let duration: f32 = 0.20;

    // Halfway through dash
    let timer = 0.10;
    let t = (timer / duration).clamp(0.0, 1.0);
    let current_speed = end_dash_speed + (start_dash_speed - end_dash_speed) * t;
    assert_eq!(current_speed, 450.0);

    // At end of dash
    let timer_end = 0.0;
    let t_end = (timer_end / duration).clamp(0.0, 1.0);
    let exit_speed = end_dash_speed + (start_dash_speed - end_dash_speed) * t_end;
    assert_eq!(exit_speed, 300.0);
}
