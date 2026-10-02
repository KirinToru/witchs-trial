<h1 align="center">Witch's Trial</h1>
<h3 align="center">A 2D Dark Fantasy Action-Platformer</h3>
<p align="center">
<img src="https://img.shields.io/badge/C++-23-00599C?style=flat-square&logo=c%2B%2B">
<img src="https://img.shields.io/badge/SFML-v3.1.0-8CC445?logo=SFML&style=flat-square">
<img src="https://img.shields.io/badge/Physics-Custom%202D%20CCD%20Engine-orange?style=flat-square">
<a href="LICENSE"><img src="https://img.shields.io/badge/license-Source%20Available-informational?style=flat-square"/></a>
</p>

**Witch's Trial** is a fast-paced 2D dark fantasy action-platformer powered by a strictly custom 2D kinematic physics engine with Continuous Collision Detection (CCD). Built with modern **C++23** and **SFML 3.1.0**, the project features zero external physics dependencies, a dual-form transformation system (Witch & Beast), Souls-like posture combat, aerial momentum mechanics, an arcane magic arsenal, and multi-phase boss encounters.

---

### Contents

- [Features](#features)
- [Controls](#controls)
- [Combat Mechanics](#combat-mechanics)
- [Tech Stack](#tech-stack)
- [Building from Source](#building-from-source)
- [Creating a Release](#creating-a-release)
- [Documentation & Roadmap](#documentation--roadmap)
- [License](#license)

---

## Features

- **Custom 2D Kinematic Physics Engine:**
  - Continuous Collision Detection (CCD) via swept AABB slab raycasting preventing high-velocity tunneling.
  - 60Hz deterministic fixed-timestep accumulator loop decoupled from variable rendering.
  - Horizontal static tile collider merging from Tiled TMX maps for ultra-fast broadphase queries.
  - Dynamic surface modifiers: low-friction **Ice** surfaces and vertical velocity reflection **Trampolines**.

- **Dual-Form Transformation (Witch & Beast):**
  - **Witch Form (Agile Caster):** High aerial agility, Stamina management with delayed regeneration, strictly horizontal Air-Dash (Left/Right), timed defensive Parry (`C`), and Arcane Gun/Spells. Passive mana regeneration was explicitly removed to enforce strict resource management (replenished via save points or combat).
  - **Beast Form (Ferocious Werewolf):** Heavy physical mass (3.5 kg), increased knockback resistance, devastating forward **Pounce** and downward **Ground Smash**, Rage meter generated strictly by dealing damage, and an area-of-effect **Roar** stagger (`L`).

- **Souls-Like Combat Foundation:**
  - Precise frame-windowed Hitbox and Hurtbox collision detection.
  - **Posture & Stagger System:** Enemies possess poise/posture meters that regenerate over time. Breaking enemy posture triggers extended Stagger and critical finisher vulnerability.
  - **3-Hit Melee Combo:** Methodical combo chaining with Melee Magnetism (forward startup pull), aerial air-stalls (gravity suspension during slashes), aerial jump/dash resets upon landing the 3rd finisher hit, and a strict 0.6s finisher cooldown lockout that enforces the methodical combat rhythm.
  - **Timed Parry Counter (`C`):** 0.20s parry window completely negates incoming damage, awards +40 Rage, forces attacking enemies into a 2.0s Stagger, and bursts metallic sparks.
  - **Beast Roar (`L`):** Consumes Rage to emit a 240px circular shockwave breaking enemy poise and inflicting a 3.5s Stagger.
  - **Offensive Dash Attack:** Dashing into enemies deals damage and launches the player with an upward bounce, consuming additional Stamina and Mana to prevent infinite spam.

- **High-Mobility & Arcane Magic Arsenal:**
  - **Arcane Gun (`K`):** Fast projectile with auto-aim enemy targeting and physical recoil impulse knocking the player backward and upward (replenishing double jump).
  - **Pogo-Magic Fireball (`L` + `J`):** Cast a slow-moving fiery orb with "Safe-Exit" AABB logic (allowing the player to safely cast, run alongside, or stand inside the orb without taking self-damage); strike it with a melee slash to trigger a massive vertical super-bounce (-820 px/s) while resetting dash cooldown and double jump.
  - **Ice Wall (`L`):** Conjure solid crystalline pillars for vertical wall-jumps and platforming.
  - **Thunder Strike (`L`):** High-voltage lightning bolt that electrifies and stuns enemies; touch to execute shock-jumps.
  - **Equipment & Grimoire Menu (`I`):** Interactive semi-transparent menu to switch equipped spells, allocate Shard upgrade points, and manage Relics.

- **Multi-Phase Boss Encounter & Hazards:**
  - **Grand Inquisitor:** Multi-phase boss encounter with telegraphed AoE ground smashes, bloodflame transformation, rapid lunges, dynamic camera zoom, and physical arena lock walls.
  - **Environmental Hazards:** Deadly **Moth Floor** infected lava tiles, ceiling-mounted **Pendulum Traps** with symplectic numerical integration, and spike pits.

- **Visual Juice & Dark Fantasy Typography:**
  - Custom pooled particle system: directional blood splatters, muzzle flash sparks, dash ghost trails, debris bursts, and shockwaves.
  - Dynamic camera shake trauma and hit-stop freeze frames on impacts and posture breaks.
  - Dark Fantasy typography (`Cinzel`) for boss titles and menus; pixel-perfect rendering (`Pixeloid`) with `setSmooth(false)` and integer rounding for HUD telemetry and floating damage popups.
  - 32-channel audio pool with spatial sound effects and streamed music.

---

## Controls

| Action | Primary Key | Secondary Key | Notes |
|---|---|---|---|
| **Move Left / Right** | `A` / `D` | `Left` / `Right` Arrow | Ground acceleration with friction |
| **Jump / Wall Jump** | `W` | `Up Arrow` | Variable jump height; resets on pogo & dash bounce |
| **Fast Fall / Drop Platform** | `S` | `Down Arrow` | Drop through one-way platforms |
| **Melee Attack Combo** | `J` | | 3-hit combo with Melee Magnetism & air-stall |
| **Arcane Gun** | `K` | | Auto-aim shot with physics recoil knockback |
| **Cast Spell / Beast Roar** | `L` | | Witch: Equipped spell (Pogo/Ice/Thunder); Beast: Roar Stagger |
| **Timed Parry** | `C` | | Witch only: 0.2s parry window with counter stun |
| **Dash / Pounce** | `Left Shift` | | Witch: Horizontal dash; Beast: Leaping pounce |
| **Ground Smash** | `S` + `Left Shift` | | Beast only (in air): Downward slam crushing props |
| **Transform Form** | `Q` | | Toggle between Witch and Beast form |
| **Equipment & Grimoire** | `I` | | Open magic and attribute upgrade menu |
| **Quick Restart (Hold)** | `R` (Hold 1.0s) | `Enter` (Game Over) | Smooth fade-to-black level reset |
| **Toggle Debug Laser/Hitboxes** | `F3` | | Real-time auto-aim raycast & collision boxes |
| **Pause Game** | `Escape` | | Open pause options overlay |

---

## Combat Mechanics

```
                       [ WITCH FORM ]
        +-------------------------------------------+
        | High Agility  |  Stamina & Mana Pools     |
        | 3-Hit Combo   |  Air-Stall Gravity Freeze |
        | Parry ('C')   |  Arcane Gun Recoil ('K')  |
        | Pogo-Magic    |  Ice Wall & Thunder Spells|
        +-------------------------------------------+
                              |
                     ['Q' Transform / 0 Rage]
                              |
                              v
                       [ BEAST FORM ]
        +-------------------------------------------+
        | Heavy Mass (3.5kg) |  Rage Meter (Decays) |
        | Heavy Claw Strikes |  Pounce & Smash      |
        | Beast Roar ('L')   |  Destroys Props      |
        +-------------------------------------------+
```

---

## Tech Stack

| Category | Technology |
|----------|------------|
| **Language Standard** | C++23 |
| **Build System** | CMake 3.28+ |
| **Multimedia Framework** | SFML 3.1.0 (Graphics, Window, System, Audio) |
| **Physics Engine** | Custom Kinematic Swept CCD 2D Engine (Zero external dependencies) |
| **Map System** | Custom Tiled TMX Parser & Dynamic Collider Generator |
| **Supported Platforms** | Windows x64 (MSVC / Clang), Linux x64 (GCC / Clang), macOS (ARM64 / x64) |

---

## Building from Source

### Prerequisites

- A **C++23** compatible compiler (MSVC 19.40+ / Visual Studio 2022 v17.10+, GCC 14+, or Clang 18+)
- **CMake** 3.28 or higher
- *Note: SFML 3.1.0 is automatically retrieved and built via CMake FetchContent.*

### Windows (Visual Studio / CMake)

```powershell
# Clone the repository
git clone https://github.com/KirinToru/physbox-2d.git
cd physbox-2d

# Configure & Build
cmake -S . -B build
cmake --build build --config Release
```

The compiled binary `WitchsTrial.exe` will be located in `build/Release/` with all required assets and runtime libraries copied automatically.

### Linux (Ubuntu / Debian)

```bash
# Install required development packages
sudo apt-get update
sudo apt-get install -y cmake g++ libxrandr-dev libxcursor-dev libudev-dev \
    libopenal-dev libflac-dev libvorbis-dev libgl1-mesa-dev libegl1-mesa-dev \
    libxi-dev libfreetype6-dev libmbedtls-dev libssh2-1-dev libharfbuzz-dev libogg-dev

# Configure & Build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

---

## Creating a Release

The repository is configured with an automated CI/CD pipeline (`.github/workflows/release.yml`) that automatically builds, packages, and attaches multi-platform binaries (Windows, Linux, macOS) to GitHub Releases upon pushing a version tag.

### Option 1: Using Git Terminal (Recommended)

To create and publish a new release tag:

```powershell
# Ensure working tree is clean and on main
git checkout main
git pull origin main

# Create an annotated git tag (e.g., v0.2.0)
git tag -a v0.2.0 -m "Release v0.2.0: Souls-like Combat, Boss Encounter & Visual Polish"

# Push the tag to GitHub to trigger the release workflow
git push origin v0.2.0
```

### Option 2: Using GitHub CLI (`gh`)

```powershell
gh release create v0.2.0 --title "v0.2.0: Combat & Visual Polish" --generate-notes
```

---

## Documentation & Roadmap

- **[Changelog](docs/changelog.md)** - Chronological log of architecture and mechanic additions
- **[Roadmap](docs/ROADMAP.md)** - Future milestones and planned features
- **[Doxygen](docs/Doxyfile)** - Codebase documentation generator

---

## License

This project is licensed under a **Source Available License**. See [LICENSE](LICENSE) for terms.  
External libraries: [SFML](https://www.sfml-dev.org/) (zlib/png license).
