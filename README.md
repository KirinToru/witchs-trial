<h1 align="center">Witch's Trial</h1>
<h3 align="center">A 2D Action-Platformer</h3>
<p align="center">
<img src="https://img.shields.io/badge/C++-23-00599C?style=flat-square&logo=c%2B%2B">
<img src="https://img.shields.io/badge/SFML-v3.1.0-8CC445?logo=SFML&style=flat-square">
<img src="https://img.shields.io/badge/Physics-Custom%202D%20Engine-orange?style=flat-square">
<a href="LICENSE"><img src="https://img.shields.io/badge/license-Source%20Available-informational?style=flat-square"/></a>
</p>

**Witch's Trial** is a 2D action-platformer featuring a strictly custom 2D kinematic physics engine built from scratch. Built with modern C++23 and SFML 3.1.0, free of any third-party physics libraries.

### Contents

- [Features](#features)
- [Controls](#controls)
- [Tech Stack](#tech-stack)
- [Build](#build)
- [Documentation](#documentation)
- [License](#license)

## Features

- **Custom 2D Kinematic Physics Engine:** Pure vector-based movement without third-party physics libraries.
- **Responsive Platformer Controller:**
  - Acceleration, ground friction, and momentum preservation.
  - Variable jump height with apex gravity reduction (hang time) and early release gravity scaling.
  - Wall sliding, fast wall sliding, and wall jumping.
  - Coyote time and jump input buffering for tight game feel.
  - Upward ceiling corner correction to prevent snagging on edges.
- **Multi-directional Dash & Air Dash:** Cardinal directional dashes with a brief freeze phase and preserved horizontal momentum.
- **Tiled Map Integration:** TMX map loading with AABB wall collisions and one-way platforms (drop-through supported).
- **Developer Tooling:** Built-in dev console (`~`), telemetry HUD (FPS, frame times, speed), and debug hitbox visualization (`F1`).

## Controls

| Action | Primary Key | Secondary Key |
|---|---|---|
| Move Left | `A` | `Left Arrow` |
| Move Right | `D` | `Right Arrow` |
| Jump / Wall Jump | `Space` | |
| Dash / Air Dash | `Left Shift` | |
| Fast Slide / Drop Platform | `S` | `Down Arrow` |
| Toggle Dev Console | `~` (Grave) | |
| Toggle Hitboxes | `F1` | |
| Toggle Telemetry HUD | `F2` | |
| Cycle Window Mode | `F4` | |
| Pause Game | `Escape` | |

## Tech Stack

| Category | Tool |
|----------|------|
| Language Standard | C++23 |
| Build System | CMake 3.28+ |
| Framework | SFML 3.1.0 |
| Physics Engine | Custom 2D Kinematic Physics Engine |
| Supported Platforms | Windows x64, Linux, macOS |

## Build

### Prerequisites

- **C++23** compatible compiler (e.g., MSVC 19.40+ / Visual Studio 2022/2026, GCC 14+, Clang 18+)
- **CMake** 3.28+
- *Note: SFML 3.1.0 is automatically downloaded and configured via CMake FetchContent.*

### Windows (Visual Studio / CMake)

```shell
cmake -S . -B build
cmake --build build --config Release
```

The executable `WitchsTrial` will be generated in the `build` directory with all assets copied post-build.

## Documentation

- **[Roadmap](docs/ROADMAP.md)** - Planned features and progress tracking
- **[Doxygen](docs/Doxyfile)** - HTML API documentation configuration

## License

This project uses a **Source Available License**. See [LICENSE](LICENSE) for full details.
This project uses the external multimedia library [SFML](https://www.sfml-dev.org/), licensed under the zlib/png license.
