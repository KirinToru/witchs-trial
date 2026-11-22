use crate::menu::GameSettings;

//------------[Verify Voxel Shader Key Serialization, Deserialization, And Fallback]-------------------
#[test]
fn test_voxel_shader_settings_serialization() {
    // 1. Serialization when enabled
    let settings = GameSettings {
        voxel_shader: true,
        ..Default::default()
    };
    let config_true = settings.to_config_string();
    assert!(config_true.contains("voxel_shader=true"));

    // 2. Serialization when disabled
    let settings_disabled = GameSettings {
        voxel_shader: false,
        ..Default::default()
    };
    let config_false = settings_disabled.to_config_string();
    assert!(config_false.contains("voxel_shader=false"));

    // 3. Deserialization from string with voxel_shader=false
    let mut loaded = GameSettings::default();
    loaded.parse_from_str("window_mode=Windowed\nvsync=true\nvoxel_shader=false\n");
    assert!(!loaded.voxel_shader);

    // 4. Deserialization from string with voxel_shader=true
    let mut loaded_true = GameSettings {
        voxel_shader: false,
        ..Default::default()
    };
    loaded_true.parse_from_str("window_mode=Windowed\nvsync=true\nvoxel_shader=true\n");
    assert!(loaded_true.voxel_shader);

    // 5. Fallback to default true when key is absent (e.g. legacy settings.cfg)
    let mut legacy_loaded = GameSettings::default();
    legacy_loaded.parse_from_str("window_mode=Windowed\nvsync=false\nfps_lock=Fps120\n");
    assert!(legacy_loaded.voxel_shader);
}
