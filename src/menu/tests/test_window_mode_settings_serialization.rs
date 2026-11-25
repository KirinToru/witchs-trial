use crate::menu::{GameSettings, WindowDisplayMode};

//------------[Verify Window Mode Serialization and Deserialization Across All Modes]-------------------
#[test]
fn test_window_mode_settings_serialization() {
    let modes = [
        (WindowDisplayMode::Windowed, "window_mode=Windowed"),
        (WindowDisplayMode::Borderless, "window_mode=Borderless"),
        (WindowDisplayMode::Exclusive, "window_mode=Exclusive"),
    ];

    for (mode, expected_line) in modes {
        // 1. Verify serialization emits exact variant name
        let settings = GameSettings {
            window_mode: mode,
            ..Default::default()
        };
        let serialized = settings.to_config_string();
        assert!(serialized.contains(expected_line));

        // 2. Verify round-trip parsing restores the exact mode
        let mut loaded = GameSettings::default();
        loaded.parse_from_str(&serialized);
        assert_eq!(loaded.window_mode, mode);
    }

    // 3. Unknown values fall back safely to Windowed
    let mut fallback = GameSettings {
        window_mode: WindowDisplayMode::Borderless,
        ..Default::default()
    };
    fallback.parse_from_str("window_mode=InvalidValue\n");
    assert_eq!(fallback.window_mode, WindowDisplayMode::Windowed);
}
