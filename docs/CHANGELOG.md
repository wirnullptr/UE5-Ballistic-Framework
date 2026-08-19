# Changelog

All notable changes to this project are documented in this file.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [1.0.0-preview] — 2026-08-19

First public preview. Binary-only distribution for **Unreal Engine 5.7 / 5.8**.

### Added

- **Core simulation** (`UBallisticFramework`) — Structure-of-Arrays layout, sparse/dense indexing with generation handles, closed-form drag solver, `ParallelFor` batch processing.
- **Dormant LOD** — projectiles beyond `DormantDistance` fully sleep and are resolved analytically back to their exact position on wake.
- **Collision-LOD** — three precision tiers (L0/L1/L2) scaled by distance, with segment tracing to prevent tunneling.
- **Ricochet & penetration** — material- and angle-driven, data-driven per `EPhysicalSurface`, with energy-based falloff.
- **Cosmetic subsystem** — batched impact FX, sound with speed-of-sound delay, pooled decals, FOV-aware culling.
- **Tracers** — two independent modes: billboard (Niagara, mass-scale) and ribbon (3D geometry, camera-facing).
- **Damage routing** — notify-by-default target component, optional point-damage fallback, and an optional GAS bridge in the separate `BallisticGAS` module (the core does not link GameplayAbilities).
- Project Settings across three sections (Core / Cosmetic / Damage Routing) and Data Asset types for projectile profiles, surface interactions, and impact FX.

### Notes

- The tracer Niagara system and ribbon-trail material ship as plugin content and are pre-assigned as defaults.
- No sample projectile / surface / FX Data Assets are included; author them per project (see [QUICKSTART.md](./QUICKSTART.md)).
