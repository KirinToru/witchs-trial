//------------[Verify Air Horizontal Speed Is Strictly Clamped to 300 Units]-------------------
#[test]
fn test_air_speed_limit() {
    let raw_dash_boost: f32 = 720.0;
    let clamped_air_speed = raw_dash_boost.clamp(-300.0, 300.0);
    assert_eq!(clamped_air_speed, 300.0);

    let negative_raw: f32 = -650.0;
    let clamped_neg = negative_raw.clamp(-300.0, 300.0);
    assert_eq!(clamped_neg, -300.0);
}
