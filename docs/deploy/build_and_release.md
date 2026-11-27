# Witch's Trial Build and Deployment

## System Requirements

- Operating System: Windows 10/11, Linux (X11/Wayland), or macOS.
- Rust toolchain: 1.85+ (stable channel).
- Graphics: GPU supporting Vulkan 1.2+, DirectX 12, Metal, or OpenGL 3.3+.

## Build and Run Commands

```bash
# Debug build and execute all 58 unit tests
cargo test

# Strict linter verification (zero warnings policy)
cargo clippy --all-targets -- -D warnings

# Build optimized release binary
cargo build --release

# Run optimized release binary
cargo run --release
```

The resulting binary will be placed in `target/release/witchs-trial` (or `witchs-trial.exe` on Windows). All `assets/` (maps, tilesets, sprites, and audio) are embedded directly into the executable via `include_dir`, so the binary runs standalone without requiring an external `assets/` folder next to it (though local disk files take priority if present). Configuration is saved to `settings.cfg` beside the executable.

## Automated GitHub Releases

Automated multi-platform standalone binaries (paired with `LICENSE` and `CREDITS.txt`) are published via GitHub Actions on tag pushes matching `v*`.
Each release includes SHA256 checksums and automatically generated release notes detailing changes between tags.
