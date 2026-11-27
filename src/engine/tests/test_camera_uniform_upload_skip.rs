use glam::Vec2;
use crate::engine::renderer::should_upload_camera_uniform;

//------------[Verify Camera Uniform Upload Is Skipped When Viewport Is Unchanged]-------------------
#[test]
fn test_camera_uniform_upload_skip() {
    let base_viewport = Vec2::new(640.0, 360.0);
    let resized_viewport = Vec2::new(1280.0, 720.0);

    // 1. Initial upload with no prior viewport cached must trigger upload
    assert!(should_upload_camera_uniform(None, base_viewport));

    // 2. Repeated calls with the same viewport size must skip GPU uniform writes
    assert!(!should_upload_camera_uniform(
        Some(base_viewport),
        base_viewport
    ));

    // 3. Changing viewport dimensions must trigger a new camera uniform upload
    assert!(should_upload_camera_uniform(
        Some(base_viewport),
        resized_viewport
    ));
}
