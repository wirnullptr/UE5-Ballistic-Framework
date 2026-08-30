# API Reference

Quick reference for every **callable** entity of the public API — functions, methods,
and events. Property/config fields are **not** duplicated here; see
[`CONFIGURATION.md`](./CONFIGURATION.md).

**Legend:** `🟦` Blueprint-callable · `⬛` C++-only.

---

## Module BallisticCore

### `UBallisticFramework` (World Subsystem — simulation core)
- 🟦 `FBallisticHandle SpawnBallisticUnit(const FBallisticSpawnParams& Params)` — spawns a projectile from the given params (profile, start position, direction, owner); returns a handle for later queries.
- 🟦 `bool IsHandleValid(const FBallisticHandle& Handle) const` — whether the projectile behind this handle is still alive (slot not reused by another projectile).
- 🟦 `FBallisticProjectileProfile GetProjectileProfile(const FBallisticHandle& Handle) const` — returns the projectile's profile (mass / drag / speed / trace type) by handle.
- 🟦 `void RegisterDamageReceiver(TScriptInterface<IBallisticDamageReceiver> Receiver)` — registers a receiver of the per-frame impact batch (typically the damage router); the core feeds all registered receivers one batch per frame.
- 🟦 `void UnregisterDamageReceiver(TScriptInterface<IBallisticDamageReceiver> Receiver)` — removes a registered receiver (call on teardown).
- 🟦 `FBallisticPoolStats GetPoolStats() const` — snapshot of pool statistics (active / dormant projectile counts) for monitoring or HUD.
- ⬛ `FOnBallisticImpactBatchNative& GetOnImpactBatchNative()` — access to the native impact-batch delegate for C++ subscription (cosmetics, damage bridges); fires once per frame on authority.
- ⬛ `int32 CopyActiveTracerPositions(TArray<FVector>& Out, int32 MaxCount) const` — copies positions of active non-dormant projectiles (for custom GPU/Niagara rendering); keeps Out capacity, returns the count written. Call once per frame after Tick.
- ⬛ `int32 CopyActiveTracerVelocities(TArray<FVector>& Out, int32 MaxCount) const` — same for velocities, index-aligned with positions (for velocity-aligned tracers).
- ⬛ `int32 CopyActiveTracerData(TArray<FVector>& OutPositions, TArray<int32>& OutSparseIndices, TArray<uint32>& OutGenerations, TArray<uint16>& OutProfileIndices, TArray<float>& OutEnergies, TArray<TWeakObjectPtr<AActor>>& OutOwners, int32 MaxCount) const` — extended one-pass snapshot (position + stable projectile ID + profile + energy + owner) for trails / custom cosmetics.

### `UBallisticCosmeticSubsystem` (World Subsystem — world cosmetics)
- ⬛ `bool TrySpendFXToken()` — spends one FX token from the shared per-frame budget; `true` if a token was available and spent (for custom FX spawners that should respect the budget).
- ⬛ `bool RegisterMuzzleFlash(const AActor* Instigator, const FVector& Location)` — deduplicates muzzle flashes within one game tick; `false` if a duplicate (same Instigator + location already this frame).

### `UBallisticCosmeticStatics` (Blueprint Function Library)
- 🟦 `static void PlayMuzzleFlash(UObject* WorldContextObject, FVector Location, FRotator Rotation, UNiagaraSystem* FlashFX, AActor* Instigator)` — plays a muzzle flash through the cosmetic subsystem (with dedup and FX budget).

### `UBallisticDamageRoutingSubsystem` (World Subsystem — damage router)
- ⬛ `void RegisterDamageTarget(AActor* Owner, TScriptInterface<IBallisticDamageTarget> Target)` — registers a damage target by actor; the router groups batch impacts by HitActor and delivers each target its own sub-batch. Usually called by the target's component in BeginPlay.
- ⬛ `void UnregisterDamageTarget(AActor* Owner)` — removes a registered target (call in EndPlay).

### `UBallisticProjectileData` (Data Asset)
- 🟦 `bool IsValidProfileIndex(int32 Index) const` — whether a projectile profile with this index exists in the table.
- 🟦 `const FBallisticProjectileProfile& GetProfile(int32 Index) const` — projectile profile by index.

### `FBallisticDragSolver` (static — closed-form ballistics)
- ⬛ `static FVector SampleVelocity(const FVector& V0, const FVector& G, float K, float T)` — projectile velocity after `T` seconds from initial `V0` under gravity `G` and linear drag `K` (closed form).
- ⬛ `static FVector SamplePosition(const FVector& X0, const FVector& V0, const FVector& G, float K, float T)` — projectile position after `T` seconds from `(X0, V0)` (closed form).

---

## Module BallisticGAS

### `UBallisticGASDamageStatics` (Blueprint Function Library)
- 🟦 `static UAbilitySystemComponent* ResolveASC(AActor* Actor)` — AI-safe ASC resolve via `UAbilitySystemGlobals::GetAbilitySystemComponentFromActor` (Pawn / PlayerState / bot uniformly, no casts).
- 🟦 `static bool ApplyBallisticDamageViaGAS(const FBallisticImpactEventLite& Impact, AActor* TargetActor, const UBallisticGASDamageConfig* Config)` — applies impact damage through GAS: maps `DamageProfileTag → {GameplayEffect, SetByCallerTag}` from Config, `MakeOutgoingSpec` + SetByCaller + Apply; returns `true` on success.

---

## Interfaces (implemented on your side)

- ⬛ `virtual void IBallisticDamageTarget::ReceiveBallisticImpacts(const TArray<FBallisticImpactEventLite>& Impacts) = 0` — implement on your actor/component to receive, from the router, the sub-batch of impacts for **this actor only**. The component only notifies — you decide whether and how to apply damage.
- ⬛ `virtual void IBallisticDamageReceiver::ApplyBallisticImpacts(const TArray<FBallisticImpactEventLite>& Impacts) = 0` — implement to register as an in-box receiver and get the whole per-frame impact batch (`RegisterDamageReceiver`).

---

## Events (`UPROPERTY(BlueprintAssignable)` — bind in BP or C++)

- 🟦 `FOnBallisticImpactBatch UBallisticFramework::OnImpactBatch` — broadcast once per frame with the WHOLE impact batch (`TArray<FBallisticImpactEventLite>`); bind for your own global hit handling.
- 🟦 `FOnBallisticImpact UBallisticFramework::OnBallisticImpact` — legacy single-impact delegate (fat `FBallisticImpactEvent` struct); kept for compatibility.
- 🟦 `FOnBallisticDamageReceived UBallisticDamageTargetComponent::OnBallisticDamageReceived` — broadcast once PER impact of this target (not batched); bind to decide how to react / apply damage (armor, crits, ignore).
- 🟦 `FOnBallisticImpactBatch UBallisticOwnerComponent::OnMyProjectileImpact` — broadcast as a BATCH of impacts only from this component owner's projectiles (personal shooter feedback: hitmarkers / UI / sound).
