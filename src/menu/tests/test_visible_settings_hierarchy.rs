use crate::menu::{MenuSystem, SettingItem};

//------------[Verify Settings Visibility Hierarchy Based on VSync and Match Timestep]-------------------
#[test]
fn test_visible_settings_hierarchy() {
    let mut menu = MenuSystem::new();

    // 1. VSync is enabled by default: MatchTimestep and FpsLock must NOT be visible (9 items)
    menu.settings.vsync = true;
    let visible = menu.visible_settings();
    assert!(!visible.contains(&SettingItem::MatchTimestep));
    assert!(!visible.contains(&SettingItem::FpsLock));
    assert!(visible.contains(&SettingItem::MusicVolume));
    assert!(visible.contains(&SettingItem::SfxVolume));
    assert_eq!(visible.len(), 9);

    // 2. VSync disabled and MatchTimestep enabled: MatchTimestep is visible, FpsLock is NOT (10 items)
    menu.settings.vsync = false;
    menu.settings.match_timestep = true;
    let visible = menu.visible_settings();
    assert!(visible.contains(&SettingItem::MatchTimestep));
    assert!(!visible.contains(&SettingItem::FpsLock));
    assert!(visible.contains(&SettingItem::MusicVolume));
    assert!(visible.contains(&SettingItem::SfxVolume));
    assert_eq!(visible.len(), 10);

    // 3. VSync disabled and MatchTimestep disabled: Both MatchTimestep and FpsLock ARE visible (11 items)
    menu.settings.vsync = false;
    menu.settings.match_timestep = false;
    let visible = menu.visible_settings();
    assert!(visible.contains(&SettingItem::MatchTimestep));
    assert!(visible.contains(&SettingItem::FpsLock));
    assert!(visible.contains(&SettingItem::MusicVolume));
    assert!(visible.contains(&SettingItem::SfxVolume));
    assert_eq!(visible.len(), 11);
}

