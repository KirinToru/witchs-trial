# Witch's Trial Technical Architecture and API Reference

Witch's Trial is a 2D retro-style precision platformer engine built from scratch in Rust using `wgpu` for hardware-accelerated graphics and `winit` for window and event handling.

## Architectural Overview

The engine is structured into six modular subsystems:

```
+-------------------------------------------------------------------+
|                            main.rs                                |
|         (winit ApplicationHandler, Main Loop, Mode Switching)     |
+-------------------------------------------------------------------+
       |                                              |
       v                                              v
+-----------------------+                    +----------------------+
|        menu           |                    |         game         |
|  (UI State Machine,   |                    | (Gameplay Director,  |
|   Settings, Screens)  |                    |  Camera, Diagnostics)|
+-----------------------+                    +----------------------+
       |                                              |
       |                   +--------------------------+
       |                   |                          |
       v                   v                          v
+-------------------------------+             +---------------------+
|            player             |             |         map         |
| (Kinematics, FSM Controller,  |             | (TMX Parser, Tiles, |
|  Sub-Stepped AABB Collisions) |             |  Merged Colliders)  |
+-------------------------------+             +---------------------+
       |                                              |
       +--------------------+    +--------------------+
                            |    |
                            v    v
                  +-----------------------+
                  |        physics        |
                  |  (AABB Intersections) |
                  +-----------------------+
                            |
                            v
                  +-----------------------+
                  |        engine         |
                  | (Renderer, Textures,  |
                  |  Input, Timers, Fonts)|
                  +-----------------------+
```

### Subsystems Breakdown

1. `engine`:
   - `WgpuRenderer`: Hardware-accelerated 2D quad and textured sprite batch renderer built on `wgpu`.
   - `InputState`: Bitmask-driven keyboard and mouse state tracking single-frame edges and continuous presses.
   - `TimeAccumulator`: Fixed-timestep accumulator ensuring deterministic physics simulation at configurable frequencies.
   - `FrameLimiter`: Precision sleep and busy-spin frame rate limiter matching monitor refresh rate or custom FPS caps.
   - `Texture`: GPU texture resource loader and bind group manager.
   - `DebugFont`: Built-in bitmap font generator for diagnostic overlays and HUD telemetry.

2. `game`:
   - Coordinates active map data, player entity, camera follow tracking, and HUD diagnostics.
   - Manages level lifecycle: initial spawn, checkpoints, bottom pit death, victory exit trigger, and death fade transitions.
   - Tracks performance telemetry: actual FPS, frame render time, simulation frequency, and window mode.

3. `player`:
   - Kinematic controller implementing variable jump height, coyote time, jump buffering, wall sliding, and air dashing.
   - Finite State Machine (`PlayerState`) driving sprite animations and kinematic branches.
   - Collision resolution module executing adaptive sub-stepped AABB checks against solid walls and one-way platforms.

4. `map`:
   - Custom XML parser for Tiled TMX map files.
   - Dynamic tileset resolution mapping arbitrary GIDs to texture UV atlas coordinates.
   - Horizontal collider merging algorithm combining adjacent solid tiles into consolidated AABB bars.
   - Decodes tile rotation flags (horizontal flip, vertical flip, diagonal flip) and transforms rotated spike hitboxes.

5. `menu`:
   - Keyboard-driven UI supporting Main, Map Select, Settings, Pause, and Level Complete screens.
   - Handles real-time adjustments of window mode, VSync, target FPS lock, timestep simulation modes, and auto-bhop toggles.

6. `physics`:
   - Lightweight Axis-Aligned Bounding Box (`AABB`) geometry primitives.
   - Intersection detection (`overlaps`) and proximity testing (`touches`).

---

## Simulation and Rendering Loop

The application loop decouples physics simulation from graphics rendering:

### Fixed-Timestep Physics with Sub-Frame Interpolation
- The physics engine runs at a fixed tick rate (configurable between 30 Hz and 300 Hz, default 60 Hz).
- Accumulator logic (`TimeAccumulator`) gathers real elapsed time each frame and executes discrete simulation steps (`game.update(dt, input)`).
- Before each physics tick, previous player position (`player.prev_pos`) and previous camera position (`game.prev_camera_pos`) are recorded.
- During rendering, the sub-frame fraction `alpha = accumulator / fixed_dt` is computed:
  ```rust
  let render_pos = prev_pos.lerp(pos, alpha);
  ```
- This ensures fluid visuals on high refresh rate monitors (120 Hz, 144 Hz, 240 Hz) without compromising deterministic gameplay.

### Dynamic Timestep Mode
- Alternatively, players can select `Dynamic (Smooth)` mode where physics steps use frame delta time directly (`dt = frame_duration.min(0.05)`), passing `alpha = 1.0` to rendering.

---

## Collision Detection and Resolution Pipeline

### Adaptive Sub-Stepping
To prevent high-speed tunneling through obstacles at lower simulation frequencies (e.g. during a 600 px/s dash at 30 Hz), movement is divided into safe sub-steps:
1. Maximum axis displacement is calculated: `max_displacement = max(|vel.x| * dt, |vel.y| * dt)`.
2. Safe threshold is 8.0 pixels per sub-step: `steps = max(1, ceil(max_displacement / 8.0))`.
3. In each sub-step of duration `sub_dt = dt / steps`:
   - Hazard pre-check: verify player hurtbox against hazard tiles.
   - Horizontal movement: displace `pos.x += vel.x * sub_dt`, resolve overlaps against solid colliders, zero `vel.x` on contact.
   - Vertical movement: displace `pos.y += vel.y * sub_dt`, resolve overlaps against solid colliders, zero `vel.y` and set `grounded = true` on floor contact.
   - One-way platforms: test downward pass-through based on position at sub-step start.
   - Hazard post-check: verify player hurtbox against hazard tiles.

### Merged Horizontal Colliders
Adjacent solid wall tiles (ID 8) and one-way platform tiles (ID 9) within the same row are merged into single widened AABB colliders during map loading:
- Eliminates "ghost vertex snagging" where entities catch on internal tile boundaries.
- Reduces collision check complexity by 70-85% compared to checking individual 32x32 tiles.
- Visual tiles remain strictly 32x32 individual quads to preserve correct texture tiling without distortion.

---

## Player Finite State Machine (FSM)

The player entity is driven by seven mutually exclusive states:

- `Idle`: Grounded with horizontal speed below 8 px/s.
- `Walk`: Grounded and accelerating/moving horizontally.
- `Jump`: Airborne with negative vertical velocity (ascending).
- `Fall`: Airborne with positive vertical velocity (descending).
- `WallSlide`: Airborne, contacting a wall while pressing toward it with downward speed clamped to a controlled slide curve.
- `DashFreeze`: 60 ms pause upon dash activation freezing movement before impulse launch.
- `Dash`: 200 ms horizontal burst tapering smoothly from 600 px/s down to 300 px/s with gravity suppressed.

---

## Automated API Documentation

Witch's Trial utilizes Rust's native automated documentation tool, `rustdoc`. All modules, structs, enums, functions, and internal methods are parsed directly from source annotations.

### Generating Interactive HTML Documentation

To build and open the interactive documentation in your default browser:

```bash
cargo doc --document-private-items --no-deps --open
```

The generated documentation is located at `target/doc/witchs_trial/index.html` and provides full-text search, type signatures, trait implementations, and direct source code links.
