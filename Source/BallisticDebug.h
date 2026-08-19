#pragma once

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Stats/Stats.h"
#include "BallisticDebug.generated.h"

// Log category
DECLARE_LOG_CATEGORY_EXTERN(LogBallistic, Log, All);

// Stats group
DECLARE_STATS_GROUP(TEXT("BallisticSubsystem"), STATGROUP_Ballistic, STATCAT_Advanced);

DECLARE_CYCLE_STAT_EXTERN(TEXT("Simulate [dispatch]"),       STAT_Ballistic_Simulate,        STATGROUP_Ballistic,);
DECLARE_CYCLE_STAT_EXTERN(TEXT("Simulate [parallel]"),       STAT_Ballistic_SimulateParallel, STATGROUP_Ballistic,);
DECLARE_CYCLE_STAT_EXTERN(TEXT("Simulate [single-thread]"),  STAT_Ballistic_SimulateST,       STATGROUP_Ballistic,);
DECLARE_CYCLE_STAT_EXTERN(TEXT("IntegrateRange"),            STAT_Ballistic_Integrate,        STATGROUP_Ballistic,);
DECLARE_CYCLE_STAT_EXTERN(TEXT("QueryRange"),                STAT_Ballistic_Query,            STATGROUP_Ballistic,);
DECLARE_CYCLE_STAT_EXTERN(TEXT("MergeResults"),              STAT_Ballistic_Merge,            STATGROUP_Ballistic,);
DECLARE_CYCLE_STAT_EXTERN(TEXT("ProcessImpacts"),            STAT_Ballistic_ProcessImpacts,   STATGROUP_Ballistic,);
DECLARE_CYCLE_STAT_EXTERN(TEXT("FlushRemovals"),             STAT_Ballistic_FlushRemovals,    STATGROUP_Ballistic,);

DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Active Projectiles"), STAT_Ballistic_ActiveCount,   STATGROUP_Ballistic,);
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Free Pool Slots"),    STAT_Ballistic_FreeSlots,     STATGROUP_Ballistic,);
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Impacts/Frame"),      STAT_Ballistic_ImpactsFrame,  STATGROUP_Ballistic,);
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Removals/Frame"),     STAT_Ballistic_RemovalsFrame, STATGROUP_Ballistic,);
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Substeps/Frame"),     STAT_Ballistic_Substeps,      STATGROUP_Ballistic,);

DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("LOD0 Queries/Frame"), STAT_Ballistic_LOD0Queries,  STATGROUP_Ballistic,);
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("LOD1 Queries/Frame"), STAT_Ballistic_LOD1Queries,  STATGROUP_Ballistic,);
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("LOD2 Queries/Frame"), STAT_Ballistic_LOD2Queries,  STATGROUP_Ballistic,);
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Skipped/Frame"),      STAT_Ballistic_LODSkipped,   STATGROUP_Ballistic,);
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Dormant (LOD3)"),     STAT_Ballistic_Dormant,      STATGROUP_Ballistic,);

DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Line Traces/Frame"),    STAT_Ballistic_LineTraces,    STATGROUP_Ballistic,);
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Sphere Traces/Frame"),  STAT_Ballistic_SphereTraces,  STATGROUP_Ballistic,);
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Capsule Traces/Frame"), STAT_Ballistic_CapsuleTraces, STATGROUP_Ballistic,);

// Косметика (Trail Ribbon) — ОТДЕЛЬНАЯ группа. Консоль: `stat BallisticCosmetic`.
DECLARE_STATS_GROUP(TEXT("BallisticCosmetic"), STATGROUP_BallisticCosmetic, STATCAT_Advanced);
DECLARE_CYCLE_STAT_EXTERN(TEXT("CopyActiveTracerData [call]"),  STAT_BallisticCosmetic_CopyTracerData, STATGROUP_BallisticCosmetic,);
DECLARE_CYCLE_STAT_EXTERN(TEXT("Trail Candidate Select+Sort"),  STAT_BallisticCosmetic_TrailSelect,    STATGROUP_BallisticCosmetic,);

// Console Variables
namespace BallisticCVars
{
    extern TAutoConsoleVariable<int32> DebugEnabled;
    extern TAutoConsoleVariable<int32> DrawTrajectories;
    extern TAutoConsoleVariable<int32> DrawCollisionSpheres;
    extern TAutoConsoleVariable<int32> DrawVelocityVectors;
    extern TAutoConsoleVariable<int32> DrawImpactPoints;
    extern TAutoConsoleVariable<int32> ShowPoolStats;
    extern TAutoConsoleVariable<int32> MaxDebugProjectiles;
    extern TAutoConsoleVariable<float> ImpactMarkerLifetime;
    extern TAutoConsoleVariable<int32> ForceSingleThread;

    // ── TraceType debug ───────────────────────────────────────────────────
    extern TAutoConsoleVariable<int32> DrawTraceTypeColors;

    // ── Simulation timing ─────────────────────────────────────────────────
    extern TAutoConsoleVariable<float> FixedTimeStep;

    // ── Trail визуализация ────────────────────────────────────────────────
    // Рисует угасающий трейл из TrailPoints точек за снарядом.
    // Каждая точка — отрезок с alpha-fade от головы к хвосту.
    // Цвет трейла: Line=синий, Sphere=бирюзовый, Capsule=оранжевый (тип),
    // или speed-градиент (зелёный→красный) если DrawTraceTypeColors=0.
    extern TAutoConsoleVariable<int32> DrawTrails;

    // History points per projectile trail. 8 = cheap, 24 = high quality.
    // Memory: TrailPoints * MaxDebugProjectiles * 12 bytes (FVector).
    extern TAutoConsoleVariable<int32> TrailPoints;

    // ── LOD-зоны в мире ──────────────────────────────────────────────────
    // Рисует кольца LOD0/1/2 вокруг ViewLocation прямо в мире.
    // Зелёное кольцо = граница LOD0→LOD1, жёлтое = LOD1→LOD2.
    // Полезно для настройки дистанций — сразу видишь где снаряды переключаются.
    extern TAutoConsoleVariable<int32> DrawLODZones;

    // ── Импакт-спарки ────────────────────────────────────────────────────
    // При попадании рисует burst из N коротких линий ("спарки") веером от нормали.
    // 0 = старое поведение (крест + стрелка), 1 = спарки включены.
    extern TAutoConsoleVariable<int32> DrawImpactSparks;

    // Spark count per impact. 8 = minimum, 16 = good, 24 = best.
    extern TAutoConsoleVariable<int32> ImpactSparkCount;

    // Длина одного спарка в см. Масштабируется с ImpactEnergy.
    extern TAutoConsoleVariable<float> ImpactSparkLength;

    // ── Скорость-цвет ─────────────────────────────────────────────────────
    // Граничная скорость для speed-градиента (cm/s).
    // При скорости >= этого значения снаряд рисуется как "максимум" (красный/яркий).
    // Default 90000 = 900 m/s. Shotgun pellets travel slower — reduce to ~30000.
    extern TAutoConsoleVariable<float> DebugMaxSpeed;
}

// Константы трейла — менять здесь, не по всему коду
namespace BallisticTrailConsts
{
    // Максимальная история точек трейла.
    // TrailPositions в SoA = MaxDebugProjectiles * MaxTrailPoints * sizeof(FVector).
    // При 512 снарядах и 24 точках = 512 * 24 * 12 = ~147KB — нормально.
    static constexpr int32 MaxTrailPoints = 24;
}

// Blueprint-friendly снапшот состояния пула
USTRUCT(BlueprintType)
struct BALLISTICCORE_API FBallisticPoolStats
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Ballistic|Debug")
    int32 ActiveProjectiles = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistic|Debug")
    int32 FreeSlots = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistic|Debug")
    int32 MaxCapacity = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistic|Debug")
    float PoolUsageFraction = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistic|Debug")
    int32 ImpactsLastFrame = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistic|Debug")
    int32 RemovalsLastFrame = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistic|Debug")
    int32 SubstepsLastFrame = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistic|Debug")
    int32 NextSparseIndex = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistic|Debug")
    int32 LOD0QueriesLastFrame = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistic|Debug")
    int32 LOD1QueriesLastFrame = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistic|Debug")
    int32 LOD2QueriesLastFrame = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistic|Debug")
    int32 LODSkippedLastFrame = 0;

    // Число спящих (LOD3_Dormant) снарядов в текущем кадре — снимок, не аккумулятор.
    UPROPERTY(BlueprintReadOnly, Category = "Ballistic|Debug")
    int32 DormantCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistic|Debug")
    int32 LineProjectiles = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistic|Debug")
    int32 SphereProjectiles = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistic|Debug")
    int32 CapsuleProjectiles = 0;
};
