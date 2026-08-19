# Configuration Reference

All settings live in `Project Settings → Plugins → Ballistic Framework`, split across three sections. Distances are in centimeters (Unreal units). The defaults listed below are the values this build ships with.

## Core

| Field | Default | Description |
| --- | --- | --- |
| `ProjectileDataAsset` | — | Data Asset holding projectile profiles. If unset, the subsystem logs an error and falls back to a single built-in default profile (it does not crash); assign your own for real content. |
| `SurfaceInteractionAsset` | — | Surface-interaction Data Asset. Unassigned → ricochet/penetration disabled. |
| `ParallelThreshold` | 64 | Projectile count above which parallel processing kicks in. |
| `BatchSize` | 256 | Batch size for `ParallelFor`. |
| `DormantDistance` | 20000 cm | Distance at which a projectile goes dormant (frozen). |
| `WakeSweepSagTolerance` | 5 cm | Precision of the retroactive check when waking from dormant. |
| `MaxWakeSegments` | 16 | Maximum number of wake-sweep segments. |
| `CollisionLOD1Fraction` | 0.33 | Fraction of `DormantDistance` within which collision runs at full precision (L0). |
| `CollisionLOD2Fraction` | 0.66 | Fraction of `DormantDistance` for the L1 → L2 transition (sparse line traces). |
| `CollisionLOD2ThrottleInterval` | 3 | On L2, trace once every N substeps. |
| `PenetrationIgnoreClearDistance` | 100 cm | After penetration, the component is ignored until the projectile travels this far (prevents re-hitting when penetrating thick walls). |
| `RicochetRandomSeed` | 0x5EED | Seed for the deterministic ricochet random stream. |
| `MaxUnits` | 100000 | Maximum simultaneously live projectiles — determines the SoA allocation size at startup. |

## Cosmetic

| Field | Default | Description |
| --- | --- | --- |
| `FXCullDistance` | 3000 cm | FX cull distance (FOV-aware). |
| `MaxFXPerSecond` | 64 | Token-bucket FX limit in real time (independent of game-time scale — works in bullet-time too). |
| `MaxDecalsInWorld` | 256 | Decal pool cap. |
| `DecalMaxVisibilityDistance` | 1000 cm | Decal visibility distance from the camera. The engine's `FadeScreenSize` cannot set a distance in cm directly, so the plugin implements its own per-frame control on top of it. |
| `PenetrationExitTraceMaxDepth` | 100 cm | Maximum object thickness searched for a penetration exit point; thicker objects produce no exit FX. |
| `SoundNearThreshold` | 5000 cm | Closer than this, sound plays without a propagation delay. |
| `SpeedOfSoundCmPerSec` | 34300 | Speed of sound used to compute the delay on distant impacts. |
| `FXLibrary` | — | Data Asset with impact-FX profiles per surface type. |
| `TracersEnabled` (`bTracersEnabled`) | false | Billboard tracers (lightweight points, Niagara bridge, CPU simulation). |
| `TracerNiagaraSystem` | `/BallisticFramework/Content/Preset/NS_BallisticTracer` (bundled) | Niagara system for billboard tracers. Cleared → tracers disabled. |
| `MaxTracerBufferSize` | 20000 | Ceiling for the tracer position buffer. |
| `TrailRibbonEnabled` (`bTrailRibbonEnabled`) | true | Ribbon trails — full 3D tail geometry, visible from any angle (including projectiles flying toward the camera, where billboard sprites are not visible). Independent of billboard tracers; both can be enabled. |
| `MaxTrailSlots` | 512 | Cap on simultaneously active ribbons (independent of `MaxTracerBufferSize`). |
| `MaxTrailPoints` | 4 | History points per ribbon. |
| `TrailCullDistance` | 20000 cm | Ribbon cull distance from the camera. |
| `TrailMinPointDistance` | 5 cm | Minimum distance between adjacent history points — avoids point pile-up on slow or stopped projectiles. |
| `TrailDefaultMaterial` | `/BallisticFramework/Content/Preset/M_BallisticTracer` (bundled) | Default ribbon material (must use Vertex Color for fade/tint). |
| `TrailDefaultWidth` | 3 | Base ribbon width. |
| `TrailEnergyWeight` | 1 | Weight of projectile energy in the trail-eviction priority when the slot pool overflows. |
| `TrailInstigatorBoost` | 10 | Priority multiplier for trails of projectiles owned by the local player when the pool overflows. |

## Damage Routing

| Field | Default | Description |
| --- | --- | --- |
| `bFallbackToPointDamage` | false | If the hit actor is not registered as a target, apply standard `ApplyPointDamage` ("out-of-the-box" damage). Ships disabled — enable it, or register a `UBallisticDamageTargetComponent`. |

---

## Data Assets

### `UBallisticProjectileData` — projectile profiles

One asset, a `Profiles` array — the array index is the `ProfileIndex` used at spawn. Key profile fields:

- **Physics**: `Mass` (kg), `LinearDrag` [0–1], `Gravity` (cm/s², vector), `Speed` (cm/s), `MaxLifeTime` (s)
- **Collision**: `CollisionRadius`, `TraceType` (Line/Sphere/Capsule), `CapsuleHalfHeight` (Capsule only)
- **LOD**: `bResolveWhileDormant` — reserved; declared but not yet used (default `false` = full freeze)
- **Interaction**: `MaxInteractions` (ricochet + penetration cap before destruction, default 3), `bCanRicochetHitInstigator`, `MinTravelBetweenInteractions`
- **Damage**: `DamageProfileTag` — tag for mapping to a GAS effect (empty = no GAS damage)
- **Trail Ribbon** (per-profile override): `TrailColor`, `TrailWidthMultiplier`

### `UBallisticSurfaceInteractionData` — surface profiles

`SurfaceProfiles` (`TMap<EPhysicalSurface, FBallisticSurfaceInteractionProfile>`) plus a `DefaultProfile` fallback. Key profile fields:

- `CriticalRicochetAngleDegrees` (default 15°) — angle below which ricochet is preferred over penetration
- `TangentialEnergyRetention` / `NormalEnergyRetention` — energy retained along/perpendicular to the surface on a bounce
- `MinEnergyToRicochet` / `RicochetChance` / `RicochetDirectionJitterDegrees`
- `bPenetrable`, `PenetrationEnergyLossFraction`, `MinEnergyToPenetrate`

### `UBallisticImpactFXLibrary` — visual/audio response

`SurfaceFX` (`TMap<EPhysicalSurface, FBallisticImpactFXEntry>`) plus a `DefaultFX` fallback. Entry fields:

- `ImpactNiagara` / `ImpactSound` / `DecalMaterial` — base set for a normal hit
- `RicochetNiagara` / `RicochetSound` / `RicochetDecalMaterial` — ricochet overrides (fall back to the base set if unset)
- `PenetrationNiagara` / `PenetrationSound` / `PenetrationDecalMaterial` — penetration overrides (same fallback; if at least one is set, an effect also spawns at the exit point)
- `DecalSize`, `DecalLifetime`
