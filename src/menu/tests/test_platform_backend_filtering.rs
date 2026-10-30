use crate::engine::GraphicsBackendOption;

//------------[Verify Platform Backend Filtering Excludes Incompatible APIs]-------------------
#[test]
fn test_platform_backend_filtering() {
    let backends = GraphicsBackendOption::available_backends();
    assert!(!backends.is_empty());
    assert!(backends.contains(&GraphicsBackendOption::Auto));

    #[cfg(target_os = "windows")]
    {
        assert!(!backends.contains(&GraphicsBackendOption::Metal));
        assert!(backends.contains(&GraphicsBackendOption::DirectX12));
        assert!(backends.contains(&GraphicsBackendOption::Vulkan));
    }

    #[cfg(target_os = "macos")]
    {
        assert!(backends.contains(&GraphicsBackendOption::Metal));
        assert!(!backends.contains(&GraphicsBackendOption::DirectX12));
    }

    #[cfg(not(any(target_os = "windows", target_os = "macos")))]
    {
        assert!(!backends.contains(&GraphicsBackendOption::Metal));
        assert!(!backends.contains(&GraphicsBackendOption::DirectX12));
        assert!(backends.contains(&GraphicsBackendOption::Vulkan));
    }
}

