# Witch's Trial Technical Architecture

## Architecture Overview

The codebase is organized into modular Rust subsystems:

- `src/main.rs`: Application entry point, window management, event loop via winit, and fixed timestep orchestration.
- `src/game.rs`: Gameplay coordinator managing the active map, player entity, camera tracking, and debug HUD overlay.
- `src/engine/`: Core engine primitives:
  - `renderer.rs`: Hardware-accelerated 2D quad/sprite batching on wgpu.
  - `input.rs`: Keyboard and mouse input state tracking.
  - `time.rs`: TimeAccumulator for fixed-timestep physics updates and sub-frame alpha interpolation.
  - `texture.rs`: GPU texture management and bind groups.
  - `debug_font.rs`: Minimal embedded 8x8 ASCII glyph font renderer.
- `src/physics/`:
  - `mod.rs`: Axis-Aligned Bounding Box (AABB) intersection and proximity detection.
  - `tests/`: Isolated unit tests for bounding box collisions.
- `src/player/`:
  - `mod.rs`: Player struct, state updates, collision resolution, and respawn logic.
  - `state.rs`: PlayerState finite state machine enum.
  - `kinematics.rs`: Kinematic calculations and numerical helpers.
  - `render.rs`: Player sprite rendering, direction flips, and visual state tints.
  - `tests/`: Dedicated unit test files for player kinematics, wall slides, dashes, and bhop physics.
- `src/map/`:
  - `mod.rs`: Tiled TMX XML parser, orientation flag decoders, and collider extraction.
  - `tests/`: Isolated tests for rotation decoders, layer data parsing, and spike hitboxes.
- `src/menu/`:
  - `mod.rs`: MenuSystem navigation, map selection, and settings management.
  - `types.rs`: Settings enums, configuration data structures, and file persistence.
  - `render.rs`: Menu UI drawing logic for main, settings, map selection, and completion screens.
  - `tests/`: Isolated tests for menu flows and setting cycling.

## Physics and Kinematics

The game runs a fixed physics loop with sub-frame interpolation:
- Horizontal ground acceleration, friction, and air drag.
- Jump buffering and coyote time for responsive jumping.
- Dynamic wall slide and directional wall jumps.
- Dash impulse with impact freeze and cooldown timer.
- Bunny hop momentum chaining with optional auto-bhop support.
- Immediate fatal collision on hazard and void modifier contact.
