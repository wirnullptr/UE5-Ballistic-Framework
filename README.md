# Ballistic Framework

**High-performance projectile simulation for Unreal Engine 5.**
Thousands of simultaneous bullets with real flight behavior, ricochet, penetration, and material-driven physics — no raycasts pretending to be bullets.

*Unreal Engine 5.7 / 5.8 · binary-only preview (v1.0).*

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

1. Drop the plugin into `[Project]/Plugins/` and restart the editor (`Edit → Plugins` → enable **Ballistic Framework**).
2. Create a `UBallisticProjectileData` asset and add at least one profile — the defaults already fly.
3. `Project Settings → Plugins → Ballistic Framework → Core` → assign it to `ProjectileDataAsset`.
4. Call `SpawnBallisticUnit` from C++ or Blueprint.

The ribbon-trail material and tracer Niagara system ship as plugin defaults, so trails are visible out of the box. Ricochet/penetration (`UBallisticSurfaceInteractionData`) and impact FX (`UBallisticImpactFXLibrary`) are optional and authored by you.

**Full walkthrough (C++ and Blueprint):** [docs/QUICKSTART.md](./docs/QUICKSTART.md)

## Documentation

- [Quick Start](./docs/QUICKSTART.md) — from install to a projectile on screen
- [Architecture & Core Mechanics](./docs/ARCHITECTURE.md) — subsystems, damage routing, LOD, ricochet, tracers
- [Configuration Reference](./docs/CONFIGURATION.md) — every Project Settings field and Data Asset field
- [Changelog](./docs/CHANGELOG.md)

## Status

v1.0 in closed testing. Networking (listen-server) and Fab release are next on the roadmap.

## Links

- Fab: *coming soon*
- Telegram devlog: [link](https://t.me/WirDev)
