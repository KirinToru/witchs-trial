# Witch's Trial Technical and API Documentation

## Automated API Documentation

Witch's Trial utilizes Rust's native automated documentation tool, `rustdoc` (the Rust ecosystem standard equivalent to Doxygen and Swagger/OpenAPI). All modules, structs, enums, functions, and internal methods are parsed directly from source code annotations and type signatures to ensure documentation is always in sync with implementation.

### Generating Interactive HTML Documentation

To build and open the interactive documentation website in your default web browser, run:

```bash
cargo doc --document-private-items --no-deps --open
```

The generated documentation is located at `target/doc/witchs_trial/index.html` and provides:
- Live client-side full-text search across all symbols, methods, and constants.
- Module hierarchy navigation (`engine`, `game`, `map`, `menu`, `physics`, `player`).
- Type signatures, trait implementations, visibility levels, and default values.
- Direct links to the underlying source code for every item.

## Architecture and Core Principles

The engine is built around deterministic simulation, data locality, and modular decoupling:

### 1. Deterministic Simulation Loop
- Fixed-timestep physics update running at configurable frequencies (30 Hz to 300 Hz).
- `TimeAccumulator` decouples physics ticks from variable monitor refresh rates.
- Sub-frame alpha interpolation eliminates visual stutter during frame rendering.

### 2. Kinematics and Continuous Collision Detection
- Swept Axis-Aligned Bounding Box (AABB) collision prevents tunneling at high velocities.
- Horizontal tile collider merging prevents ghost-vertex edge snagging.
- Forgiving platformer mechanics: Coyote time (120ms), jump buffering (160ms), and wall slides.
- Dedicated hurtbox sizing decoupled from collision geometry.

### 3. Rendering Architecture
- Low-overhead batch renderer built directly on `wgpu`.
- Dynamic backend selection supporting Vulkan, DirectX 12, Metal, and OpenGL.
- Texture atlas mapping with nearest-neighbor point sampling for retro pixel aesthetics.
- Camera transformations applied in screen space to prevent GPU uniform buffer race conditions.
