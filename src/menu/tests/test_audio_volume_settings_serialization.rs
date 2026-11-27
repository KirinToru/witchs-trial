use crate::menu::GameSettings;

//------------[Verify Audio Volume Serialization Clamping and Fallback]-------------------
#[test]
fn test_audio_volume_settings_serialization() {
    // 1. Verify default volume values
    let defaults = GameSettings::default();
    assert_eq!(defaults.music_volume, 70);
    assert_eq!(defaults.sfx_volume, 80);

    // 2. Verify serialization and round-trip parsing for normal values
    let custom = GameSettings {
        music_volume: 40,
        sfx_volume: 90,
        ..Default::default()
    };
    let serialized = custom.to_config_string();
    assert!(serialized.contains("music_volume=40"));
    assert!(serialized.contains("sfx_volume=90"));

    let mut loaded = GameSettings::default();
    loaded.parse_from_str(&serialized);
    assert_eq!(loaded.music_volume, 40);
    assert_eq!(loaded.sfx_volume, 90);

    // 3. Verify clamping for values above 100
    let mut clamped = GameSettings::default();
    clamped.parse_from_str("music_volume=150\nsfx_volume=999\n");
    assert_eq!(clamped.music_volume, 100);
    assert_eq!(clamped.sfx_volume, 100);

    // 4. Verify invalid or negative values keep safe defaults
    let mut fallback = GameSettings::default();
    fallback.parse_from_str("music_volume=invalid\nsfx_volume=-10\n");
    assert_eq!(fallback.music_volume, 70);
    assert_eq!(fallback.sfx_volume, 80);
}
