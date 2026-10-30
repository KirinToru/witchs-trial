use crate::menu::{MenuSystem, SettingItem};

//------------[Verify Settings Visibility Hierarchy Based on VSync and Match Timestep]-------------------
#[test]
fn test_visible_settings_hierarchy() {
    let mut menu = MenuSystem::new();

    // 1. VSync is enabled by default: MatchTimestep and FpsLock must NOT be visible
    menu.settings.vsync = true;
    let visible = menu.visible_settings();
    assert!(!visible.contains(&SettingItem::MatchTimestep));
    assert!(!visible.contains(&SettingItem::FpsLock));

    // 2. VSync disabled and MatchTimestep enabled: MatchTimestep is visible, FpsLock is NOT
    menu.settings.vsync = false;
    menu.settings.match_timestep = true;
    let visible = menu.visible_settings();
    assert!(visible.contains(&SettingItem::MatchTimestep));
    assert!(!visible.contains(&SettingItem::FpsLock));

    // 3. VSync disabled and MatchTimestep disabled: Both MatchTimestep and FpsLock ARE visible
    menu.settings.vsync = false;
    menu.settings.match_timestep = false;
    let visible = menu.visible_settings();
    assert!(visible.contains(&SettingItem::MatchTimestep));
    assert!(visible.contains(&SettingItem::FpsLock));
}

