# Quick Start

From an empty project to a projectile flying on screen. Target engine: **Unreal Engine 5.7 / 5.8**.

## Prerequisites

1. Copy the plugin folder into `[YourProject]/Plugins/BallisticFramework/`.
2. Open the project, then `Edit → Plugins`, find **Ballistic Framework**, enable it, and restart the editor.
3. This is a binary-only build: it ships compiled for the engine version above. A mismatched engine version reports an incompatibility rather than silently rebuilding.

## What ships vs. what you create

**Included and pre-wired (works out of the box):**

- The ribbon-trail material (`TrailDefaultMaterial`) and the tracer Niagara system (`TracerNiagaraSystem`) are bundled inside the plugin (`/BallisticFramework/Content/…`) and assigned as defaults.
- Ribbon trails are enabled by default (`bTrailRibbonEnabled = true`), so a spawned projectile leaves a visible trail with no extra setup.

**Not bundled — you create these (by design):**

- `UBallisticProjectileData` — projectile profiles. Needed for real profiles (step 1).
- `UBallisticSurfaceInteractionData` — ricochet/penetration rules. *Optional.*
- `UBallisticImpactFXLibrary` — impact FX / sound / decals. *Optional.*

No sample Data Assets are shipped: profiles, surfaces, and impact FX are project-specific content, so you author them for your game.

## Step 1 — Create a projectile profile

1. Content Browser → right-click → *Miscellaneous → Data Asset* → choose `BallisticProjectileData`.
2. Open it and add one entry to the `Profiles` array. The array index is the `ProfileIndex` you spawn with — the first entry is index `0`.
3. The profile defaults already produce a working bullet: `Mass 0.01`, `Speed 90000` (≈ 900 m/s), `Gravity (0,0,-980)`, `LinearDrag 0.01`, `TraceType Sphere`, `CollisionRadius 2`. Adjust as needed — every field is documented in [CONFIGURATION.md](./CONFIGURATION.md#data-assets).

## Step 2 — Assign it in Project Settings

`Project Settings → Plugins → Ballistic Framework → Core → ProjectileDataAsset` → select the asset from step 1.

If left unset, the subsystem logs an error and falls back to a single built-in default profile (it does not crash) — assign your own asset for real content.

## Step 3 — Spawn a projectile

The simulation lives in the world subsystem `UBallisticFramework`. Fill an `FBallisticSpawnParams`, call `SpawnBallisticUnit`, and it returns an `FBallisticHandle`.

`FBallisticSpawnParams` fields:

- `ProfileIndex` — row in `Profiles`
- `StartPosition` — muzzle location
- `Direction` — normalized fire direction
- `Owner` — the shooter; ignored by its own traces
- `SpeedMultiplier` — per-shot scale over the profile speed (default `1.0`)

### C++

```cpp
if (UBallisticFramework* Ballistic = GetWorld()->GetSubsystem<UBallisticFramework>())
{
    FBallisticSpawnParams Params;
    Params.ProfileIndex    = 0;                    // row in ProjectileDataAsset->Profiles
    Params.StartPosition   = MuzzleLocation;
    Params.Direction       = MuzzleForwardVector;  // normalized
    Params.Owner           = this;                 // shooter; ignored by its own traces
    Params.SpeedMultiplier = 1.f;

    const FBallisticHandle Handle = Ballistic->SpawnBallisticUnit(Params);
}
```

### Blueprint

`SpawnBallisticUnit` is `BlueprintCallable`, and `FBallisticSpawnParams` / `FBallisticHandle` are Blueprint types:

1. `Get World Subsystem` → class `BallisticFramework`.
2. `Make FBallisticSpawnParams` → set `ProfileIndex`, `StartPosition`, `Direction`, `Owner`, `SpeedMultiplier`.
3. `Spawn Ballistic Unit` (target = the subsystem) → returns the handle.

## Step 4 — See it fly

- With defaults, the projectile already leaves a ribbon trail (enabled by default).
- To confirm trajectory and impacts visually, use the debug-draw console variables:
  - `Ballistic.Debug.Enabled 1`
  - `Ballistic.Debug.DrawTrajectories 1`
  - `Ballistic.Debug.DrawImpactPoints 1`

## Optional next steps

- **Ricochet & penetration** — create a `UBallisticSurfaceInteractionData`, define per-`EPhysicalSurface` profiles (plus a `DefaultProfile`), and assign it to `Core → SurfaceInteractionAsset`. Unassigned → projectiles are destroyed on impact instead of bouncing or penetrating.
- **Impact FX / sound / decals** — create a `UBallisticImpactFXLibrary`, fill `SurfaceFX` (plus `DefaultFX`), and assign it to `Cosmetic → FXLibrary`.
- **Damage** — add a `UBallisticDamageTargetComponent` to actors that should react to hits and bind `OnBallisticDamageReceived`, or enable the point-damage fallback. See [ARCHITECTURE.md](./ARCHITECTURE.md#damage-system).
- **Billboard tracers** — enable `Cosmetic → bTracersEnabled` (the tracer Niagara system is already assigned).

## Troubleshooting

- **Nothing visible after spawn** — confirm `ProjectileDataAsset` is assigned and has at least one profile; enable `Ballistic.Debug.Enabled 1` + `Ballistic.Debug.DrawTrajectories 1`, or verify `bTrailRibbonEnabled` is on.
- **Projectile spawns but has no trail** — `bTrailRibbonEnabled` must be on (default) and `TrailDefaultMaterial` assigned (default). If you cleared it, reassign `/BallisticFramework/Content/Preset/M_BallisticTracer`.
- **No ricochet / penetration** — `SurfaceInteractionAsset` is unassigned, or the hit surface has no matching profile and `DefaultProfile` disallows it.
- **No damage reaction** — this build ships with `bFallbackToPointDamage = false`; enable it in `Damage Routing`, or add a `UBallisticDamageTargetComponent` and handle `OnBallisticDamageReceived`.
- **Plugin won't enable / reports incompatibility** — the binary build targets a specific engine version; use a matching engine (see Prerequisites).
