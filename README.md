# Witch's Trial

Witch's Trial is a 2D retro-style precision platformer built from scratch in Rust with custom hardware-accelerated rendering.

## Overview

The game emphasizes tight, responsive platforming kinematics, deterministic physics simulation, and modern multi-backend graphics:

- Pure 2D platformer kinematics featuring variable jump heights, coyote time, jump buffering, wall sliding, air dashing, and bunny hop momentum chaining.
- Continuous collision detection using Swept AABB and merged horizontal collider generation to prevent ghost vertex snagging.
- Custom rendering pipeline built on wgpu, supporting Vulkan, DirectX 12, Metal, and OpenGL.
- Fixed-timestep physics simulation with configurable tick rates (30 Hz to 300 Hz) and sub-frame interpolation.
- Full keyboard-driven navigation across Main, Map Select, Settings, Pause, and Level Complete screens.

## Documentation

Comprehensive project documentation is organized into dedicated guides:

- [Player Manual and Controls](docs/user/manual.md): Gameplay mechanics, complete keyboard control mapping, and in-game settings.
- [Technical and API Documentation](docs/tech/architecture.md): Automated code documentation (rustdoc), architectural design, and subsystem reference.
- [Build and Deployment Guide](docs/deploy/build_and_release.md): Prerequisites, build steps, test execution, and CI/CD release workflow.

## Community and Policies

- [Contributing Guidelines](CONTRIBUTING.md): Coding standards, testing requirements, and pull request procedures.
- [Security Policy](SECURITY.md): Vulnerability reporting and disclosure guidelines.

## License

This project is licensed under the GNU General Public License v3.0. See [LICENSE](LICENSE) for details.
