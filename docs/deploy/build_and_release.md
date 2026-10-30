# Witch's Trial Build and Deployment

## System Requirements

- Operating System: Windows 10/11, Linux (X11/Wayland), or macOS.
- Rust toolchain: 1.85+ (stable channel).
- Graphics: GPU supporting Vulkan 1.2+, DirectX 12, Metal, or OpenGL 3.3+.

## Build and Run Commands

```bash
# Debug build and test
cargo test

# Build optimized release binary
cargo build --release

# Run optimized release binary
cargo run --release
```

The resulting binary will be placed in `target/release/witchs-trial` (or `witchs-trial.exe` on Windows).

## Automated GitHub Releases

Automated multi-platform binaries are published via GitHub Actions on tag pushes matching `v*`.
Each release includes SHA256 checksums and automatically generated release notes detailing changes between tags.
