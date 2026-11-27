# Witch's Trial

Witch's Trial is a 2D retro-style precision platformer built from scratch in Rust with custom hardware-accelerated rendering.

## Overview

The game emphasizes tight, responsive platforming kinematics, deterministic physics simulation, and modern multi-backend graphics:

- Pure 2D platformer kinematics featuring variable jump heights, coyote time, jump buffering, wall sliding, air dashing with smooth taper, and bunny hop momentum chaining.
- Strict wall-jump limit preventing continuous climbing on the same vertical wall surface until grounded.
- The Beast transformation: collecting the Moon transforms the player into The Beast, granting increased speed and jump force, wall sticking, and 2.0s invulnerability (i-frames) with knockback on spike damage instead of instant death.
- Ice tile mechanics: slippery surfaces with low friction and acceleration, maintaining full horizontal momentum upon airborne jump.
- Pushable crates (`PushableBox`): 2D physics with sub-stepping, realistic friction slide deceleration upon key release, multi-box contiguous chain pushing, immunity to lethal spikes, and wall blocking.
- Classic PlayStation 1 (PSX) post-processing shader: 15-bit RGB555 color quantization, 4x4 ordered Bayer dithering, soft scanlines, and subtle corner vignette, with an in-game toggle in Settings.
- Sub-stepped discrete AABB collision detection and merged horizontal collider generation to prevent ghost vertex snagging.
- Custom rendering pipeline built on `wgpu`, supporting Vulkan, DirectX 12, Metal, and OpenGL.
- Disk-and-embedded audio subsystem powered by `rodio 0.23.0` (`wav`/`vorbis`), supporting looping menu/level music with pause control, cached SFX triggers with per-event cooldowns, independent `Music Volume` / `SFX Volume` sliders (0-100%), and silent headless fallback.
- Standalone single-executable distribution via `include_dir`: all `assets/` are compiled directly into the binary with optional local disk override support and `settings.cfg` stored alongside the executable.
- Fixed-timestep physics simulation with configurable tick rates (30 Hz to 300 Hz) and sub-frame interpolation, alongside dynamic timestep mode.
- Full keyboard navigation across Main, Map Select, Settings, Pause, and Level Complete screens.

## Controls

| Key | Action |
| --- | --- |
| `A` / `D` or `Left` / `Right` | Move Left / Right |
| `Space` | Jump / Wall Jump / Bunny Hop |
| `Shift` | Air Dash |
| `S` or `Down` | Drop through one-way platforms |
| `W` or `Up` | Interact with checkpoints and level exit |
| `R` | Respawn / Hold to restart level |
| `F3` | Toggle Diagnostic HUD and collider overlays |
| `F4` | Quick cycle window mode (Windowed, Borderless, Exclusive) |
| `Esc` | Pause / Back to menu |

## Testing and Code Quality

The project enforces strict code quality and unit test coverage adhering to the 1-test-per-file standard:

```bash
# Run all 59 unit tests
cargo test

# Run strict linter checks
cargo clippy --all-targets -- -D warnings

# Verify compilation
cargo check
```

## Documentation

Comprehensive project documentation is organized into dedicated guides:

- [Player Manual and Controls](docs/user/manual.md): Gameplay mechanics, complete keyboard control mapping, and in-game settings.
- [Technical and API Documentation](docs/tech/architecture.md): Automated code documentation (rustdoc), architectural design, and subsystem reference.
- [Build and Deployment Guide](docs/deploy/build_and_release.md): Prerequisites, build steps, test execution, and CI/CD release workflow.

## Community and Policies

- [Contributing Guidelines](CONTRIBUTING.md): Coding standards, testing requirements, and pull request procedures.
- [Security Policy](SECURITY.md): Vulnerability reporting and disclosure guidelines.

## License and Credits

This project is licensed under the GNU General Public License v3.0. See [LICENSE](LICENSE) for details and [CREDITS.txt](CREDITS.txt) for third-party audio attributions.
