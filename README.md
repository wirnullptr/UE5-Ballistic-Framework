# Ballistic Framework

**High-performance projectile simulation for Unreal Engine 5.**
Thousands of simultaneous bullets with real flight behavior, ricochet, penetration, and material-driven physics — no raycasts pretending to be bullets.

<!-- ![Benchmark](docs/media/benchmark.gif) -->
<!-- ![Ricochet demo](docs/media/ricochet.gif) -->

---

## Why

Most projectile systems make you choose: **scale** (thousands of instances, cheap raycasts) or **behavior** (real trajectory, ricochet, penetration — but slow).

Ballistic Framework closes that gap with a custom SoA simulation core and a closed-form drag solver — positions and velocities are computed analytically instead of stepped frame-by-frame, which is what makes the scale possible without giving up real physics.

## Features

- ⚡ **SoA-based simulation** — sparse/dense indexing, generation handles, `ParallelFor` batch processing
- 😴 **Dormant LOD** — projectiles beyond a distance threshold fully sleep (zero cost), analytically resolved back to exact position the instant they matter again
- 🎯 **Collision-LOD** — trace precision scales with distance (L0/L1/L2), segment tracing prevents tunneling even on throttled tiers
- 🔫 **Ricochet & Penetration** — material- and angle-driven, energy-based damage falloff on every bounce
- 💥 **Damage-agnostic core** — GAS-free by default, works with `TakeDamage`, custom logic, or optional GAS bridge
- ✨ **Cosmetic subsystem** — impact FX, muzzle flash, distance-delayed sound, decal pooling, FOV-aware culling
- 🌈 **GPU Tracers** — billboard (cheap, mass-scale) and Ribbon (3D geometry, camera-facing) trail modes

## Performance

Tested on an empty scene, no FX, same frame budget:

| | Projectiles |
|---|---|
| Without LOD | ~33,000 |
| With LOD | ~98,500 |

## Quick Start

1. Copy the plugin into `[Project]/Plugins/`
2. `Edit → Plugins` → enable **Ballistic Framework** → restart editor
3. Create a `UBallisticProjectileData` asset, define at least one profile
4. `Project Settings → Ballistic Framework → Core` → assign it to `ProjectileDataAsset`
5. Call `SpawnBallisticUnit` (C++ or Blueprint) with the profile index

Optional: assign `UBallisticSurfaceInteractionData` for ricochet/penetration, and `UBallisticImpactFXLibrary` for visual feedback.

## Documentation

- [Architecture & Core Mechanics](./docs/ARCHITECTURE.md) — subsystems, damage routing, LOD, ricochet, tracers
- [Configuration Reference](./docs/CONFIGURATION.md) — every Project Settings field, Data Asset field

## Status

v1.0 in closed testing. Networking (listen-server) and Fab release are next on the roadmap.

## Links

- Fab: *coming soon*
- Telegram devlog: *link*
- Discord: *link*
