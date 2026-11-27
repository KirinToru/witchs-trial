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
                  |  (AABB Intersections, |
                  |   Pushable Crates)    |
                  +-----------------------+
                            |
                            v
                  +-----------------------+
                  |        engine         |
                  | (Renderer, PSX Shader,|
                  |  Input, Timers, Fonts)|
                  +-----------------------+
```

### Subsystems Breakdown

1. `engine`:
   - `WgpuRenderer`: Hardware-accelerated 2D quad and textured sprite batch renderer built on `wgpu`.
   - `PsxUniforms` & `psx_post.wgsl`: Fullscreen post-processing pass applying authentic 15-bit RGB555 color quantization, 4x4 ordered Bayer dithering, soft scanlines, and subtle corner vignette over the rendered world and menu. Bypassed completely when disabled.
   - `AudioSystem`: Disk-and-embedded audio engine built on `rodio 0.23.0` (`wav`/`vorbis`), streaming looping music (`Menu`, `Level`) with pause control, caching 12 `SfxEvent` cues in memory with per-event cooldowns, and falling back silently when no audio hardware is present.
   - `assets`: Hybrid resource loader powered by `include_dir`, embedding the entire `assets/` tree into the binary while checking disk overrides relative to CWD and executable location first.
   - `InputState`: Bitmask-driven keyboard state tracking single-frame edges and continuous presses.
   - `TimeAccumulator`: Fixed-timestep accumulator ensuring deterministic physics simulation at configurable frequencies (and safely resetting in Menu, Pause, and Dynamic mode).
   - `FrameLimiter`: Precision sleep and busy-spin frame rate limiter matching monitor refresh rate or custom FPS caps.
   - `Texture`: GPU texture resource loader and bind group manager using `assets::load_bytes`.
   - `DebugFont`: Built-in bitmap font generator for diagnostic overlays and HUD telemetry.

2. `game`:
   - Coordinates active map data, player entity, pushable crates, camera follow tracking, and HUD diagnostics.
   - Manages level lifecycle: initial spawn, checkpoints, bottom pit death, victory exit trigger, and death fade transitions.
   - Updates crates first via `PushableBox::update_all`, then updates the player via `Player::update`.
   - Separates world rendering batches (subject to PSX post-processing) from UI rendering batches (F3 diagnostic HUD rendered crisp).

3. `player`:
   - Kinematic controller implementing variable jump height, coyote time, jump buffering, wall sliding, single wall-jump limit against the same wall, and air dashing.
   - Form transformation: supports Witch form and The Beast form with enhanced movement multipliers, wall sticking, and 2.0s damage i-frames with knockback on spike collision.
   - Collision resolution module executing adaptive sub-stepped AABB checks against solid walls, one-way platforms, spikes, void pits, ice surfaces, and pushable crates with chain pushing.

4. `physics`:
   - `AABB`: Lightweight Axis-Aligned Bounding Box geometry primitives with intersection detection (`overlaps`) and proximity testing (`touches`).
   - `PushableBox`: 2D physics entity with gravity, terminal velocity, ground friction (500.0 px/s^2), ice friction (110.0 px/s^2), sub-stepping, wall collisions, chain pushing, and realistic slide deceleration.

5. `map`:
   - Custom XML/CSV parser for Tiled TMX map files loaded via `assets::load_string`.
   - Dynamic tileset resolution mapping arbitrary GIDs to texture UV atlas coordinates.
   - Horizontal collider merging algorithm combining adjacent solid tiles, platforms, and ice surfaces into consolidated AABB bars.
   - Decodes tile rotation flags (horizontal flip, vertical flip, diagonal flip) and transforms rotated spike hitboxes.

6. `menu`:
   - Keyboard-driven UI supporting Main, Map Select, Settings, Pause, and Level Complete screens.
   - Handles real-time adjustments of window mode, VSync, target FPS lock, timestep simulation modes, auto-bhop toggles, PSX post-processing shader, Music/SFX volume sliders, and GPU backend selection.
   - Persistent settings storage via `settings.cfg` resolved alongside the executable (`settings_file_path()`).

---

## Simulation and Rendering Loop

The application loop decouples physics simulation from graphics rendering:

### Fixed-Timestep Physics with Sub-Frame Interpolation
- The physics engine runs at a fixed tick rate (configurable between 30 Hz and 300 Hz, default 60 Hz).
- Accumulator logic (`TimeAccumulator`) gathers real elapsed time each frame and executes discrete simulation steps (`game.update(dt, input)`).
- Before each physics tick, previous player position (`player.prev_pos`), crate positions (`krate.prev_pos`), and previous camera position (`game.prev_camera_pos`) are recorded.
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
   - Pushable crates: resolve horizontal contact with crates, calculate overlap penetration to prevent double-movement, push contiguous crate chains, and apply slide velocity.
   - Vertical movement: displace `pos.y += vel.y * sub_dt`, resolve overlaps against solid colliders, zero `vel.y` and set `grounded = true` on floor contact.
   - One-way platforms: test downward pass-through based on position at sub-step start.
   - Hazard post-check: verify player hurtbox against hazard tiles.

### Merged Horizontal Colliders
Adjacent solid wall tiles (ID 8), one-way platform tiles (ID 9), and ice tiles (ID 11) within the same row are merged into single widened AABB colliders during map loading:
- Eliminates "ghost vertex snagging" where entities catch on internal tile boundaries.
- Reduces collision check complexity by 70-85% compared to checking individual 32x32 tiles.
- Visual tiles remain strictly 32x32 individual quads to preserve correct texture tiling without distortion.

---

## Automated API Documentation

Witch's Trial utilizes Rust's native automated documentation tool, `rustdoc`. All modules, structs, enums, functions, and internal methods are parsed directly from source annotations.

### Generating Interactive HTML Documentation

To build and open the interactive documentation in your default browser:

```bash
cargo doc --document-private-items --no-deps --open
```

The generated documentation is located at `target/doc/witchs_trial/index.html` and provides full-text search, type signatures, trait implementations, and direct source code links. The standalone project passes 58 headless unit tests across all subsystems.
