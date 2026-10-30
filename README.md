# Witch's Trial

Witch's Trial is a retro-style precision platformer with 2D physics-based mechanics.

## Overview

The game features deterministic 2D platforming kinematics, dynamic Timestep and Physics calculations, multiple renderers with help of wgpu API, Tile map rendering, customizable display and simulation settings, and retro pixel-art shaders.

## Controls

- `[A]` / `[D]` or `Left` / `Right` - Move Left / Right
- `[Space]` - Jump
- `[Shift]` - Dash
- `[S]` or `Down` - Drop down
- `[W]` or `Up` - Interaction
- `[R]` - Reset to checkpoint / level spawn
- `[F3]` - Toggle debug overlays and diagnostic HUD
- `[F4]` - Toggle fullscreen
- `[Escape]` - Pause / return to menu

## Technical Stack

- Language: Rust (2024 edition)
- Graphics: wgpu (Vulkan, DirectX 12, Metal, OpenGL)
- Windowing and Input: winit
- Map Format: Tiled XML (.tmx)
- Assets: PNG textures with nearest-neighbor sampling

## Building and Running

```bash
# Verify compilation and tests
cargo test

# Run optimized release build
cargo run --release
```

## License

This project is licensed under the GNU General Public License v3.0 (GNU GPL-3.0). See LICENSE for details.
