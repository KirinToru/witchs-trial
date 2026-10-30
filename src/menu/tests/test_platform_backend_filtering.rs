use crate::engine::GraphicsBackendOption;

//------------[Verify Platform Backend Filtering Excludes Incompatible APIs]-------------------
#[test]
fn test_platform_backend_filtering() {
    let backends = GraphicsBackendOption::available_backends();
    #[cfg(target_os = "windows")]
    {
        assert!(!backends.contains(&GraphicsBackendOption::Metal));
        assert!(backends.contains(&GraphicsBackendOption::DirectX12));
        assert!(backends.contains(&GraphicsBackendOption::Vulkan));
    }
}

