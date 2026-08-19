#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "Async/ParallelFor.h"
#include "Engine/StreamableManager.h"
#include "Engine/HitResult.h"                    // FHitResult (сигнатуры SweepSegment/QueryRange)
#include "CollisionQueryParams.h"               // FCollisionQueryParams (те же сигнатуры)
#include "Data/BallisticData.h"
#include "Data/BallisticProjectileData.h"
#include "Data/BallisticSurfaceInteractionData.h"
#include "BallisticDebug.h"
#include "BallisticWorkerContext.h"
#include "BallisticEventTypes.h"
#include "BallisticDamageReceiver.h"
#include "BallisticSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBallisticImpact, const FBallisticImpactEvent&, ImpactEvent);

// Внутренняя структура результата sync-трейса
struct FBallisticSimResult
{
    int32      DenseIndex = INDEX_NONE;
    bool       bExpired   = false;
    bool       bHit       = false;
    FHitResult HitResult;

    // L2 throttled-хит: скорость на момент ПЕРЕСЕЧЕНИЯ (аппрокс. направлением
    // сегмента × текущий модуль). Если взведён — MergeResults берёт её вместо
    // живой Velocities[i] (source of truth для будущего рикошета).
    bool       bHasImpactVelocityOverride = false;
    FVector    OverrideImpactVelocity     = FVector::ZeroVector;
};

//~ UBallisticFramework — ядро симуляции снарядов (WorldSubsystem + FTickableGameObject).
//~ Конфиг — UBallisticFrameworkCoreSettings (Project Settings), читается в Initialize.
UCLASS()
class BALLISTICCORE_API UBallisticFramework : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    // И-5: net mode ненадёжен в ShouldCreateSubsystem — создаёмся всегда,
    // а тяжёлую SoA-аллокацию гейтим по authority в OnWorldBeginPlay.
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;

    // World-guard: тикаем только когда мир в фазе begin-play под authority
    // (bIsSimulationAuthority выставляется в OnWorldBeginPlay, сбрасывается в
    // Deinitialize) и есть живые снаряды. Не тикаем на клиенте и при teardown.
    virtual bool IsTickable() const override
    {
        return bIsSimulationAuthority && ActiveUnitCount > 0;
    }

public:
    // Legacy-делегат импакта (толстая структура).
    UPROPERTY(BlueprintAssignable)
    FOnBallisticImpact OnBallisticImpact;

    // ── Мульти-ресивер damage-канал (§7, И-8) ──────────────────────────────────
    // Ядро раздаёт батч импактов тремя каналами и не знает, кто как считает урон:
    //   1) native-делегат (C++ подписчики: cosmetic, мосты),
    //   2) BP-делегат OnImpactBatch (1 вызов/кадр),
    //   3) реестр IBallisticDamageReceiver (GAS / simple / сторонний мост).

    // Native-канал: C++ подписка на батч импактов.
    FOnBallisticImpactBatchNative& GetOnImpactBatchNative() { return OnImpactBatchNative; }

    // BP-канал: назначается в Blueprint, вызывается раз в кадр всем батчем.
    UPROPERTY(BlueprintAssignable, Category = "Ballistics|Events")
    FOnBallisticImpactBatch OnImpactBatch;

    // Реестр приёмников урона. Мост регистрируется здесь (обычно в BeginPlay под
    // authority), ядро кормит всех зарегистрированных одним батчем.
    UFUNCTION(BlueprintCallable, Category = "Ballistics|Events")
    void RegisterDamageReceiver(TScriptInterface<IBallisticDamageReceiver> Receiver);

    UFUNCTION(BlueprintCallable, Category = "Ballistics|Events")
    void UnregisterDamageReceiver(TScriptInterface<IBallisticDamageReceiver> Receiver);

    // ── DataAsset (runtime, из UBallisticFrameworkCoreSettings в Initialize) ─────────────
    // TSoftObjectPtr — не держит сильную ссылку, не блокирует загрузку карты.
    // Резолвится синхронно в Initialize() через FStreamableManager из Settings.
    TSoftObjectPtr<UBallisticProjectileData> ProjectileDataAsset;

    // Карта EPhysicalSurface -> профиль рикошета/пробития (Фаза 3). Опционален:
    // не назначен -> снаряд уничтожается при попадании (поведение до рикошета).
    TSoftObjectPtr<UBallisticSurfaceInteractionData> SurfaceInteractionAsset;

    // ── Публичный API ─────────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "Ballistic")
    FBallisticHandle SpawnBallisticUnit(const FBallisticSpawnParams& Params);

    UFUNCTION(BlueprintCallable, Category = "Ballistic")
    bool IsHandleValid(const FBallisticHandle& Handle) const;

    // Возвращает профиль снаряда по его handle — для BP логики урона и эффектов.
    // Если handle невалиден — вернёт пустой профиль и залогирует Warning.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ballistic")
    FBallisticProjectileProfile GetProjectileProfile(const FBallisticHandle& Handle) const;

    // ── Трейсеры (Веха 3) — read-only снапшот для GPU-Niagara моста ─────────────
    // Копирует позиции активных НЕ-Dormant снарядов в Out (Out.Reset() — держит
    // ёмкость вызывающего, ноль лишних аллокаций при стабильном размере). Проход по
    // dense-диапазону [0, ActiveUnitCount), пропуск LOD3_Dormant, обрезка по MaxCount.
    // Read-only, вызывать раз в кадр ПОСЛЕ Tick на game thread — НЕ hot-path
    // (Simulate/QueryRange не трогаются). Возвращает число записанных элементов.
    int32 CopyActiveTracerPositions(TArray<FVector>& Out, int32 MaxCount) const;

    // Зеркало CopyActiveTracerPositions, но копирует Velocities[i] (для velocity-
    // aligned трейсеров). ТОТ ЖЕ dense-проход/пропуск LOD3_Dormant/обрезка → элемент k
    // выровнен с k-м из CopyActiveTracerPositions при вызове подряд без мутации SoA.
    int32 CopyActiveTracerVelocities(TArray<FVector>& Out, int32 MaxCount) const;

    // Расширенный снапшот для ribbon-трейлов v2.0 (§4.4): четыре СИНХРОННЫХ по
    // dense-индексу массива за один проход — позиция + стабильный ID снаряда
    // (sparse+generation, переживает репак dense) + профиль (для per-profile стиля).
    // Тот же контракт: dense-проход [0, ActiveUnitCount), пропуск LOD3_Dormant, обрезка
    // MaxCount, Reset()+Reserve() на всех выходах. Read-only, НЕ hot-path. Отдельный от
    // CopyActiveTracerPositions/Velocities (билборд v1.0 их не меняет). Возвращает
    // число записанных элементов (одинаковое во всех четырёх массивах).
    int32 CopyActiveTracerData(
        TArray<FVector>&  OutPositions,
        TArray<int32>&    OutSparseIndices,
        TArray<uint32>&   OutGenerations,
        TArray<uint16>&   OutProfileIndices,
        TArray<float>&    OutEnergies,
        TArray<TWeakObjectPtr<AActor>>& OutOwners,
        int32 MaxCount) const;

    // ── Конфиг-члены (runtime, наполняются из UBallisticFrameworkCoreSettings в Initialize) ──
    // Ниже — прежние config-UPROPERTY, теперь обычные runtime-члены. Источник правды
    // — UBallisticFrameworkCoreSettings (Project Settings). Дефолты здесь = дефолты Settings
    // (перезаписываются в Initialize; оставлены как безопасный фолбэк).

    // ── Threading ─────────────────────────────────────────────────────────────
    int32 ParallelThreshold = 64;
    int32 BatchSize = 256;

    // ── LOD (dormant, веха 1) ─────────────────────────────────────────────────
    // Дистанция ухода в dormant-заморозку (см). 15000 = 150м.
    float DormantDistance = 15000.f;
    // Макс. провис хорды сегмента wake-sweep (см).
    float WakeSweepSagTolerance = 5.f;
    // Потолок числа сегментов retroactive wake-sweep.
    int32 MaxWakeSegments = 16;

    // ── Collision-LOD (веха-B, доли от DormantDistance, точка отсчёта — Pawn) ────
    float CollisionLOD1Fraction = 0.33f;
    float CollisionLOD2Fraction = 0.66f;
    // L2: трейс раз в N substep'ов (сегментом, анти-туннелинг).
    int32 CollisionLOD2ThrottleInterval = 3;

    // После пробития компонент игнорируется, пока снаряд не отойдёт дальше (см).
    float PenetrationIgnoreClearDistance = 100.f;

    // ── Debug API ─────────────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "Ballistic|Debug")
    FBallisticPoolStats GetPoolStats() const;

    UFUNCTION(BlueprintCallable, Category = "Ballistic|Debug")
    void Debug_SetSimulationPaused(bool bPaused);

    UFUNCTION(BlueprintCallable, Category = "Ballistic|Debug")
    void Debug_KillAllProjectiles();

    UFUNCTION(BlueprintCallable, Category = "Ballistic|Debug")
    bool Debug_TeleportProjectile(const FBallisticHandle& Handle, const FVector& NewPosition);

    UFUNCTION(BlueprintCallable, Category = "Ballistic|Debug")
    void Debug_LogProjectileState(const FBallisticHandle& Handle) const;

private:
    // Аллоцирует все SoA-массивы под MaxUnits. Вызывается из OnWorldBeginPlay
    // ТОЛЬКО под authority (И-5) — на NM_Client SoA не аллоцируется.
    void AllocateSoA();

    // ── Simulation pipeline ───────────────────────────────────────────────────
    void Simulate(float DeltaTime);
    void SimulateParallel(float DeltaTime);
    void SimulateSingleThreaded(float DeltaTime);

    // Per-batch контексты для RunParallelPass — persistent между кадрами,
    // чтобы не аллоцировать TArray'и внутри TrailPoints каждый тик.
    TArray<FBallisticWorkerContext> BatchContexts;

    // Общий шаблон параллельного прохода (И-1): каждый батч пишет только в свой
    // FBallisticWorkerContext, в общие структуры — только serial Merge после ParallelFor.
    // Не ParallelForWithTaskContext — проще, ~390 контекстов при 100k/256 это копейки.
    template <typename FBatchBody, typename FMerge>
    void RunParallelPass(int32 NumBatches, FBatchBody&& BatchBody, FMerge&& Merge)
    {
        if (BatchContexts.Num() < NumBatches)
        {
            BatchContexts.SetNum(NumBatches);
        }
        for (int32 b = 0; b < NumBatches; ++b)
        {
            BatchContexts[b].Reset();
        }

        ParallelFor(NumBatches,
            [this, &BatchBody](int32 BatchIndex) { BatchBody(BatchContexts[BatchIndex], BatchIndex); },
            EParallelForFlags::Unbalanced);

        for (int32 b = 0; b < NumBatches; ++b)
        {
            Merge(BatchContexts[b]); // serial, гонки нет
        }
    }

    void IntegrateRange(FBallisticWorkerContext& Ctx, int32 Begin, int32 End, float DeltaTime);
    void MergeIntegrateContext(FBallisticWorkerContext& Ctx);

    // ── Dormant LOD (веха 1) ──────────────────────────────────────────────────
    // Serial classify в начале Simulate: далёкие снаряды замораживаем (снапшот
    // x0,v0,t0), приближающиеся — будим retroactive wake-sweep'ом. Serial-проход
    // (не в ParallelFor) — пишет в общий SoA напрямую, гонки нет (И-1).
    void UpdateDormancy(float DeltaTime);

    // Retroactive пробуждение: K-сегментный свип ВДОЛЬ ДУГИ (не хордой!) по
    // аналитическому решению FBallisticDragSolver. Попадание во сне -> импакт +
    // удаление; чисто -> оживление в аналитической позиции/скорости, LOD0_Full.
    void PerformWakeUpSweep(int32 DenseIndex);

    // Трейс явно заданного типа (радиус/капсула — из профиля). Ядро для
    // collision-LOD: L1 понижает тип, L2 форсит Line.
    bool SweepSegment(UWorld* World, const FVector& Start, const FVector& End,
                      EBallisticTraceType TraceType, const FBallisticProjectileProfile& Profile,
                      const FCollisionQueryParams& Params, FHitResult& OutHit) const;

    // Перегрузка с типом из профиля — существующие вызовы (PerformWakeUpSweep).
    bool SweepSegment(UWorld* World, const FVector& Start, const FVector& End,
                      const FBallisticProjectileProfile& Profile,
                      const FCollisionQueryParams& Params, FHitResult& OutHit) const;

    // Кэш позиций релевантных Pawn'ов — раз в кадр (Tick), не на снаряд.
    void CachePawnLocations();

    // Мин. квадрат дистанции до ближайшего Pawn. Нет Pawn'ов -> 0 (=> L0, полная
    // точность: не режем трейсы, когда точку отсчёта измерить нечем).
    FORCEINLINE float DistanceToNearestPawnSq(const FVector& Pos) const
    {
        float Best = 0.f;
        bool bFirst = true;
        for (const FVector& Pawn : CachedPawnLocations)
        {
            const float DSq = FVector::DistSquared(Pos, Pawn);
            if (bFirst || DSq < Best) { Best = DSq; bFirst = false; }
        }
        return Best; // пусто -> 0
    }

    void QueryRange(int32 Begin, int32 End,
                    UWorld* World,
                    const FCollisionQueryParams& BaseParams,
                    TArray<FBallisticSimResult>& OutResults);

    void MergeResults(const TArray<FBallisticSimResult>& Results);

    void ProcessImpactEvents();

    // Раздаёт накопленный за кадр батч импактов трём каналам (§7): native → BP →
    // реестр приёмников. Зовётся один раз в конце Tick после всей substep-логики.
    void BroadcastImpactBatch();

    void FlushPendingRemovals();
    void RemoveUnit(int32 SparseIndex);

    FVector GetViewLocation(const APlayerController* ForPC = nullptr) const;

    // Загружает ProjectileDataAsset синхронно. Вызывается из Initialize().
    // Если ассет не назначен или не грузится — логируем Error и работаем без профилей
    // (SpawnBallisticUnit будет фоллбечить на дефолтный профиль).
    void LoadProjectileDataAsset();

    // ── Debug internals ───────────────────────────────────────────────────────
    void Debug_DrawProjectiles(UWorld* World) const;
    void Debug_DrawOnScreenStats() const;
    void Debug_DrawImpactMarker(const FBallisticImpactEvent& Event, UWorld* World) const;
    void Debug_UpdateEngineStats() const;

    void Debug_DrawTrail(UWorld* World, int32 DenseIndex, int32 NumTrailPts) const;
    void Debug_DrawLODZones(UWorld* World) const;
    void Debug_PushTrailPoint(int32 DenseIndex, const FVector& Pos);

    static FColor Debug_SpeedColor(float SpeedSq, float MaxSpeedSq);
    static FColor Debug_TraceTypeColor(EBallisticTraceType Type);

public:
    // ── Capacity (runtime, из UBallisticFrameworkCoreSettings в Initialize) ──────────────
    // Clamp [1024, 1000000] выполняется в Initialize(). Источник — Project Settings.
    int32 MaxUnits = 100000;

private:
    // ── DataAsset runtime ptr ─────────────────────────────────────────────────
    // Сырой указатель — безопасен т.к. DataAsset держится через TSoftObjectPtr
    // (strong ref после LoadSynchronous) + SubSystem живёт не дольше World.
    // Не делай UPROPERTY — это не UObject-владение, это кэш.
    const UBallisticProjectileData* CachedProjectileData = nullptr;

    // Кэш surface-конфига (рикошет/пробитие). nullptr -> фича выключена.
    // Держится через SurfaceInteractionAsset (strong ref после LoadSynchronous).
    const UBallisticSurfaceInteractionData* CachedSurfaceData = nullptr;

    // Профиль поверхности по EPhysicalSurface (или DefaultProfile). Вызывать только
    // при валидном CachedSurfaceData.
    const FBallisticSurfaceInteractionProfile& GetSurfaceInteractionProfile(EPhysicalSurface Surface) const;

    // Решает физику снаряда на попадании (рикошет/пробитие/уничтожение). Меняет
    // SoA (скорость/энергия/позиция/счётчик), сбрасывает collision-LOD расписание.
    // Возвращает true если снаряд ВЫЖИЛ (рикошет/пробитие), false -> уничтожить.
    // OutImpactType — характер взаимодействия для косметики (Normal по умолчанию;
    // Ricochet/Penetration выставляются в соответствующих ветках).
    bool ResolveProjectileInteraction(int32 DenseIndex, const FHitResult& Hit, const FVector& ImpactVel,
                                      EBallisticImpactType& OutImpactType);

    // Кэш профилей — плоский массив, копия Profiles из DataAsset.
    // Зачем копия? Чтобы не прыгать по указателю на DataAsset в хот-пасе.
    // DataAsset может теоретически выгрузиться (hot reload, garbage collect) —
    // с копией нам на это параллельно.
    TArray<FBallisticProjectileProfile> CachedProfiles;

    // Single access point to a projectile's profile from the hot loop: one call,
    // one lookup into the cache-friendly flat array. Do not index
    // CachedProfiles[ProfileIndices[i]] directly at call sites — route through here.
    FORCEINLINE const FBallisticProjectileProfile& GetProfile(int32 DenseIndex) const
    {
        return CachedProfiles[ProfileIndices[DenseIndex]];
    }

    // ── SoA arrays ────────────────────────────────────────────────────────────

    TArray<FVector>  Positions;
    TArray<FVector>  OldPositions;
    // Позиция на момент ПОСЛЕДНЕГО реального трейса. На L0/L1 обновляется каждый
    // кадр; на throttled-L2 — только когда снаряд реально трейсился, поэтому
    // sweep LastCheckedPositions→Positions автоматически покрывает весь путь за
    // пропущенные кадры (анти-туннелинг сегментом, ноль отдельного массива).
    TArray<FVector>  LastCheckedPositions;
    TArray<FVector>  Velocities;
    TArray<float>    Energies;      // вычисляется, не из профиля
    TArray<float>    LifeTimes;
    // Время (MyWorldTime) последнего реального трейса — throttle-таймер L2.
    TArray<float>    LastTraceWorldTime;

    // Один массив индексов профиля вместо ~8 параллельных массивов статических
    // данных (2 байта на снаряд). Статика читается через CachedProfiles[ProfileIndices[i]].
    TArray<uint16>   ProfileIndices;

    TArray<TWeakObjectPtr<AActor>> Owners;
    // Активный collision-LOD тир: 0=L0, 1=L1, 2=L2 (репёрпоуз мёртвого
    // CollisionLODCounters). НЕ путать с LODTiers (dormant-статус — отдельная ось).
    TArray<uint8>    CollisionLODTier;
    TArray<uint8>    PendingKillFlags;

    // ── Interaction SoA (рикошет/пробитие, Фаза 3) ─────────────────────────────
    TArray<uint8>    InteractionCount;    // рикошеты + пробития вместе, один счётчик
    TArray<FVector>  LastInteractionPos;  // точка последнего взаимодействия (MinTravel)
    // Только что ПРОБИТЫЙ компонент — игнорируется трейсом, пока снаряд не отойдёт
    // на PenetrationIgnoreClearDistance. Чинит толщино-зависимость пробития
    // (повторный хит той же геометрии). Компонент, не актор — составной объект не
    // становится целиком прозрачным из-за пробития одной детали.
    TArray<TWeakObjectPtr<UPrimitiveComponent>> PenetratedComponent;
    TArray<FVector>  PenetratedAtPos;     // точка пробития (референс для distance-clear)

    // ── Dormant LOD SoA (веха 1) ──────────────────────────────────────────────
    // Спящий снаряд не интегрируется — его состояние восстанавливается аналитически
    // из снапшота (x0,v0,t0) на момент засыпания через FBallisticDragSolver.
    TArray<EBallisticLODTier> LODTiers;
    TArray<FVector>  DormantEnterPos;   // x0 — позиция на момент засыпания
    TArray<FVector>  DormantEnterVel;   // v0 — скорость на момент засыпания
    TArray<float>    DormantEnterTime;  // t0 — MyWorldTime на момент засыпания

    // ── Allocation ────────────────────────────────────────────────────────────
    TArray<uint32>   Generations;
    TArray<int32>    FreeIndices;
    TArray<int32>    SparseToDense;
    TArray<int32>    DenseToSparse;
    int32            NextSparseIndex = 0;

    // ── Per-batch result storage (sync path) ──────────────────────────────────
    TArray<FBallisticSimResult> SyncQueryResults;

    // ── Events (legacy — толстая структура) ────────────────────────────────────
    TArray<FBallisticImpactEvent> ImpactEvents;
    TSet<int32>                   PendingRemovals;

    // ── Мульти-ресивер damage-канал (§7) ───────────────────────────────────────
    // Native-делегат хранится тут; BP-делегат OnImpactBatch — публичный UPROPERTY.
    FOnBallisticImpactBatchNative OnImpactBatchNative;

    // Облегчённые импакты, накопленные за все substep'ы кадра; раздаются один раз
    // в BroadcastImpactBatch и ресетятся (ёмкость держим).
    TArray<FBallisticImpactEventLite> PendingImpacts;

    // Реестр приёмников урона. UPROPERTY: GC зануляет ссылки на уничтоженные
    // объекты, поэтому IsValid ловит «мёртвых» без dangling (безопасная подчистка).
    UPROPERTY()
    TArray<TScriptInterface<IBallisticDamageReceiver>> DamageReceivers;

    int32   ActiveUnitCount       = 0;
    float   SimulationAccumulator = 0.f;
    FVector CachedViewLocation    = FVector::ZeroVector;

    // Позиции релевантных Pawn'ов, кэш раз в кадр (не на снаряд). Точка отсчёта
    // collision-LOD — геймплейная, работает без камеры (dedicated server).
    TArray<FVector> CachedPawnLocations;

    // Authority + world-lifecycle gate (И-5). true только между OnWorldBeginPlay
    // (при NetMode != NM_Client) и Deinitialize. Гейтит аллокацию SoA, тик и спаун:
    // на клиенте SoA не аллоцируется, симуляция не идёт.
    bool    bIsSimulationAuthority = false;

    // Собственные симуляционные часы — тикают в Simulate на fixed-timestep.
    // Основа dormant-снапшотов: t0 = MyWorldTime на момент засыпания,
    // при пробуждении разность (MyWorldTime - t0) идёт в аналитический солвер.
    double  MyWorldTime           = 0.0;

    // ── Рикошет: детерминированный рандом (шанс + jitter направления) ───────────
    // Seed из UBallisticFrameworkCoreSettings (наполняется в Initialize). Один seed на
    // подсистему — заготовка под сетевую детерминированность (Веха 4). Поток
    // трогается только в serial-фазе (ResolveProjectileInteraction ← MergeResults),
    // не в ParallelFor (И-1).
    int32 RicochetRandomSeed = 0x5EED;

    // Инициализируется в Initialize() из RicochetRandomSeed. Не UPROPERTY —
    // FRandomStream не рефлектится, это чистое runtime-состояние.
    FRandomStream RicochetRandomStream;

    // ── Debug state ───────────────────────────────────────────────────────────
    bool  bSimulationPaused         = false;
    int32 DebugImpactsLastFrame     = 0;
    int32 DebugRemovalsLastFrame    = 0;
    int32 DebugSubstepsLastFrame    = 0;

    int32 DebugLOD0QueriesLastFrame  = 0;
    int32 DebugLOD1QueriesLastFrame  = 0;
    int32 DebugLOD2QueriesLastFrame  = 0;
    int32 DebugLODSkippedLastFrame   = 0;

    int32 DebugLineTracesLastFrame   = 0;
    int32 DebugSphereTracesLastFrame = 0;
    int32 DebugCapsuleTracesLastFrame= 0;

    // Снимок числа спящих (LOD3_Dormant) снарядов — пишется в UpdateDormancy,
    // это абсолютный count текущего кадра, не аккумулятор.
    int32 DebugDormantCountLastFrame = 0;

    // ── Trail SoA (только !UE_BUILD_SHIPPING) ─────────────────────────────────
#if !UE_BUILD_SHIPPING
    TArray<FVector> TrailPositions;
    TArray<uint8>   TrailHeads;
    TArray<uint8>   TrailSizes;
    int32           TrailCapacity = 0;
#endif
};
