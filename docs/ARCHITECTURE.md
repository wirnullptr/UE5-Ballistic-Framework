# Architecture & Core Mechanics

## Overview

The plugin is built from three independent subsystems:

- **Core** (`UBallisticFramework`) — flight simulation, collision, ricochet, and penetration. GAS-free and damage-agnostic: the core knows nothing about the damage system or cosmetics.
- **Cosmetic** (`UBallisticCosmeticSubsystem`) — visual and audio response: decals, particles, sound, tracers.
- **Damage Routing** (`UBallisticDamageRoutingSubsystem`) — routes damage to targets, optionally integrating with GAS through the separate `BallisticGAS` module, with no mandatory dependency.

The core is built on a SoA (Structure of Arrays) layout with sparse/dense indexing and generation handles, parallel processing via `ParallelFor`, and an adaptive LOD system that lowers collision and cosmetic cost as projectiles move away from the player.

---

## Damage system

Damage Routing is designed to **notify**, not auto-apply: by default the system reports a hit but does not deal damage itself, ruling out double application by construction.

### Basic setup

1. Add a `UBallisticDamageTargetComponent` to any actor that should take damage.
2. Bind `OnBallisticDamageReceived` (a `BlueprintAssignable` delegate) — broadcast once per impact within a sub-batch.
3. In the handler, decide what to do with the hit: call `ApplyPointDamage`, apply a GAS effect, run custom logic, or ignore it.

If no `UBallisticDamageTargetComponent` is registered, `ApplyPointDamage` is applied as a fallback ("out-of-the-box" damage) when `bFallbackToPointDamage` is `true` in Damage Routing Settings. **Note:** this build ships with `bFallbackToPointDamage = false`, so enable it — or register a target component — to receive damage.

### Automatic Point Damage without code

If you would rather not bind the delegate manually, set `bAutoApplyPointDamage = true` on the component itself: after the broadcast, `ApplyPointDamage` is additionally called automatically. Default `false`.

### GAS integration (optional)

The core and the base Damage Routing are fully GAS-free — this integration is wired in only when needed, through the separate `BallisticGAS` module.

1. Create a `UBallisticGASDamageConfig` (Data Asset) — a mapping table `DamageProfileTag → FBallisticGASDamageEntry` (`DamageEffect` — which `GameplayEffect` to apply; `SetByCallerTag` — the tag the impact's damage magnitude is written into).
2. In the `OnBallisticDamageReceived` handler, call `UBallisticGASDamageStatics::ApplyBallisticDamageViaGAS(Impact, Target, Config)`.
3. `DamageProfileTag` comes from the projectile profile (`FBallisticProjectileProfile::DamageProfileTag`) — an empty tag means the projectile is not bound to GAS damage.

The target's ASC (`AbilitySystemComponent`) is resolved only through the standard `GetAbilitySystemComponentFromActor` — no manual cast to a specific character class.

---

## Key mechanics

### Collision-LOD

Three collision-precision tiers (L0/L1/L2) based on the fraction of `DormantDistance` from the player, plus a full freeze (dormant) beyond `DormantDistance`. Distance is measured from the Pawn, not the camera.

### Ricochet & penetration

Direction and angle determine the outcome (see `CriticalRicochetAngleDegrees`). The number of interactions per projectile is capped by the per-profile `MaxInteractions` (default 3); once reached, the projectile is destroyed. Penetration of thick objects is guarded against re-triggering via `PenetrationIgnoreClearDistance`.

### FX subsystem

Impacts are batched per tick, culled by FOV and distance, and prioritized by energy and distance when `MaxFXPerSecond` is exceeded. Decals use a pool with priority-based eviction; their visibility range is controlled independently of the engine's `FadeScreenSize`.

### Tracers — two independent modes

- **Billboard** (`bTracersEnabled`) — cheap sprite points, Niagara, CPU-driven. Suited to scenes with a large number of simultaneous shots.
- **Ribbon** (`bTrailRibbonEnabled`) — full 3D geometry with a dedicated C++ renderer, visible from any angle. More expensive; suited to close-ups and realistic shooters. When the pool overflows, display priority accounts for distance to the camera and whether the projectile belongs to the local player.

The full list of configurable parameters is in [`CONFIGURATION.md`](./CONFIGURATION.md).
