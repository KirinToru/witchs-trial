use crate::menu::GameSettings;

//------------[Verify PSX Shader Key Serialization, Deserialization, And Fallback]-------------------
#[test]
fn test_psx_shader_settings_serialization() {
    // 1. Serialization when enabled
    let settings = GameSettings {
        psx_shader: true,
        ..Default::default()
    };
    let config_true = settings.to_config_string();
    assert!(config_true.contains("psx_shader=true"));

    // 2. Serialization when disabled
    let settings_disabled = GameSettings {
        psx_shader: false,
        ..Default::default()
    };
    let config_false = settings_disabled.to_config_string();
    assert!(config_false.contains("psx_shader=false"));

    // 3. Deserialization from string with psx_shader=false
    let mut loaded = GameSettings::default();
    loaded.parse_from_str("window_mode=Windowed\nvsync=true\npsx_shader=false\n");
    assert!(!loaded.psx_shader);

    // 4. Deserialization from string with psx_shader=true
    let mut loaded_true = GameSettings {
        psx_shader: false,
        ..Default::default()
    };
    loaded_true.parse_from_str("window_mode=Windowed\nvsync=true\npsx_shader=true\n");
    assert!(loaded_true.psx_shader);

    // 5. Fallback to default true when key is absent or legacy voxel key is present
    let mut legacy_loaded = GameSettings::default();
    legacy_loaded.parse_from_str("window_mode=Windowed\nvsync=false\nvoxel_shader=false\n");
    assert!(legacy_loaded.psx_shader);
}
