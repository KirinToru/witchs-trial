# Witch's Trial Player Manual

## Gameplay Overview

Navigate through precision platforming maps, avoid lethal void tiles and hazards, solve movement puzzles with pushable crates, activate checkpoints, and touch the level exit portal.

### Mechanics and Entities

1. **Witch Form (Default)**:
   - Balanced horizontal running speed (250 px/s) and jump height.
   - Air dash burst with 60 ms freeze impact and velocity taper.
   - Wall sliding with speed clamping on vertical surfaces.
   - Strict 1 wall-jump limit against the same wall surface until resetting on solid ground.
   - Bunny hop momentum preservation when timing jumps upon landing.

2. **The Beast Form**:
   - Activated upon touching the Moon collectible item.
   - Augmented horizontal movement (+25% speed, +10% jump height).
   - Wall sticking allows gripping walls indefinitely.
   - Spike immunity buffer: on spike contact, rather than dying, reverts to Witch form with 2.0s invulnerability (i-frames) and knockback impulse.

3. **Ice Surfaces**:
   - Low friction and gradual acceleration.
   - Jumping off ice maintains full horizontal velocity in the air without artificial speed capping until landing or counter-steering.

4. **Pushable Crates (`PushableBox`)**:
   - Heavy wooden crates that can be pushed horizontally by the player or shoved with dash momentum.
   - Pushing multiple crates in a contiguous row (chain pushing).
   - Realistic friction slide deceleration: releasing the push key allows the crate to smoothly coast and decelerate under ground or ice friction.
   - Crates safely land on lethal spikes without taking damage, serving as movable stepping blocks.

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

## Settings and Configuration

The in-game Settings menu allows adjusting:

- **Window Mode**: Windowed, Borderless Fullscreen, Exclusive Fullscreen.
- **VSync**: Enable or disable vertical synchronization.
- **Match Timestep**: Automatically synchronizes target frame rate with the physics simulation frequency.
- **Lock FPS**: Unlimited, 30, 60, 90, 120, 144, 165, 180, 240, 300 FPS.
- **Timestep Mode**: Dynamic or fixed tick rates (30 Hz to 300 Hz) with sub-frame interpolation.
- **Auto Bunny Hop**: Automatically buffers continuous jumps when holding Space.
- **PSX Shader**: Toggles the classic PlayStation 1 post-processing effect featuring 15-bit color quantization, 4x4 ordered Bayer dithering, soft scanlines, and subtle corner vignette (zero GPU overhead when disabled).
- **Graphics Backend**: Auto, Vulkan, DirectX 12, Metal, OpenGL (applies on game restart).
