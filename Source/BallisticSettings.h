#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "BallisticSettings.generated.h"

// Единая категория в Project Settings для всех трёх разделов плагина.
#define BALLISTIC_SETTINGS_CATEGORY TEXT("Ballistic Framework")

class UBallisticProjectileData;
class UBallisticSurfaceInteractionData;
class UBallisticImpactFXLibrary;
class UNiagaraSystem;
class UMaterialInterface;

// Конфигурация ядра Ballistic Framework (Project Settings).
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Core"))
class BALLISTICCORE_API UBallisticFrameworkCoreSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    virtual FName GetCategoryName() const override { return FName(BALLISTIC_SETTINGS_CATEGORY); }

    UBallisticFrameworkCoreSettings()
    {
        // D2: значения из живого Config/DefaultGame.ini на момент экстракции —
        // ini-секции удалены, дефолты живут здесь. Content-указатели не трогаем.
        DormantDistance                = 20000.f;
        PenetrationIgnoreClearDistance = 100.f;
    }

    // ── Data ────────────────────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Data",
        meta = (ToolTip = "DataAsset с профилями снарядов. Обязателен — без него SpawnBallisticUnit падает."))
    TSoftObjectPtr<UBallisticProjectileData> ProjectileDataAsset;

    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Data",
        meta = (ToolTip = "DataAsset взаимодействия с поверхностями. Не назначен — рикошет/пробитие выключены."))
    TSoftObjectPtr<UBallisticSurfaceInteractionData> SurfaceInteractionAsset;

    //~ ── Threading ─────────────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Threading")
    int32 ParallelThreshold = 64;

    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Threading")
    int32 BatchSize = 256;

    // ── LOD (dormant) ─────────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, config, Category = "Ballistic|LOD", meta = (ClampMin = "100.0",
        ToolTip = "Дистанция ухода снаряда в dormant-заморозку в см. 15000 = 150м."))
    float DormantDistance = 15000.f;

    UPROPERTY(EditAnywhere, config, Category = "Ballistic|LOD", meta = (ClampMin = "0.1",
        ToolTip = "Макс. провис хорды сегмента wake-sweep в см. Меньше = больше сегментов = точнее."))
    float WakeSweepSagTolerance = 5.f;

    UPROPERTY(EditAnywhere, config, Category = "Ballistic|LOD", meta = (ClampMin = "1", ClampMax = "64",
        ToolTip = "Максимальное число сегментов retroactive wake-sweep."))
    int32 MaxWakeSegments = 16;

    // ── Collision-LOD ─────────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, config, Category = "Ballistic|CollisionLOD", meta = (ClampMin = "0.0", ClampMax = "1.0",
        ToolTip = "Доля DormantDistance: ближе неё — L0 (полный трейс профиля)."))
    float CollisionLOD1Fraction = 0.33f;

    UPROPERTY(EditAnywhere, config, Category = "Ballistic|CollisionLOD", meta = (ClampMin = "0.0", ClampMax = "1.0",
        ToolTip = "Доля DormantDistance: между Frac1 и Frac2 — L1; дальше — L2 (Line, throttled)."))
    float CollisionLOD2Fraction = 0.66f;

    UPROPERTY(EditAnywhere, config, Category = "Ballistic|CollisionLOD", meta = (ClampMin = "1", ClampMax = "32",
        ToolTip = "L2 throttle: трейс раз в N substep'ов сегментом от последней трассы."))
    int32 CollisionLOD2ThrottleInterval = 3;

    // ── Interaction ───────────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Interaction", meta = (ClampMin = "0.0",
        ToolTip = "После пробития компонент игнорируется, пока снаряд не отойдёт дальше этой дистанции (см)."))
    float PenetrationIgnoreClearDistance = 100.f;

    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Interaction",
        meta = (ToolTip = "Seed детерминированного потока рикошета (шанс отскока + разброс направления)."))
    int32 RicochetRandomSeed = 0x5EED;

    // ── Capacity ──────────────────────────────────────────────────────────────
    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Capacity",
        meta = (ClampMin = "1024", ClampMax = "1000000",
                ToolTip = "Максимальное число одновременно живых снарядов. Определяет размер SoA-аллокации при старте."))
    int32 MaxUnits = 100000;
};

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Cosmetic"))
class BALLISTICCORE_API UBallisticCosmeticSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    virtual FName GetCategoryName() const override { return FName(BALLISTIC_SETTINGS_CATEGORY); }

    UBallisticCosmeticSettings()
    {
        // D2: значения из живого Config/DefaultGame.ini на момент экстракции.
        FXCullDistance               = 3000.f;
        MaxDecalsInWorld             = 256;
        DecalMaxVisibilityDistance   = 1000.f;
        PenetrationExitTraceMaxDepth = 100.f;
        bTracersEnabled              = false;
        MaxTracerBufferSize          = 20000;
        bTrailRibbonEnabled          = true;
        MaxTrailSlots                = 512;
        MaxTrailPoints               = 4;
        TrailCullDistance            = 20000.f;
        TrailDefaultWidth            = 3.f;

        // Content, перенесённый ВНУТРЬ плагина (/BallisticFramework/Content/…) —
        // путь стабилен и едет с плагином, поэтому дефолты зашиваем (пересмотр D2).
        // Soft-ref через FSoftObjectPath: без синхронной загрузки в CDO.
        // Остальные 3 (ProjectileDataAsset, SurfaceInteractionAsset, FXLibrary) —
        // вне плагина, остаются пустыми (D2 без изменений).
        TracerNiagaraSystem  = FSoftObjectPath(TEXT("/BallisticFramework/Content/Preset/NS_BallisticTracer.NS_BallisticTracer"));
        TrailDefaultMaterial = FSoftObjectPath(TEXT("/BallisticFramework/Content/Preset/M_BallisticTracer.M_BallisticTracer"));
    }

    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Cosmetic")
    float FXCullDistance = 5000.f;              // см, при FOV=90

    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Cosmetic")
    int32 MaxFXPerSecond = 64;                  // token-bucket по реальному времени

    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Cosmetic")
    int32 MaxDecalsInWorld = 500;

    // Дальность видимости декалей от камеры (см). Дальше — не рисуются. 0 = без ограничения.
    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Cosmetic", meta = (ClampMin = "0.0"))
    float DecalMaxVisibilityDistance = 5000.f;

    // Макс. толщина объекта для поиска точки выхода при пробитии (см). Толще — exit-FX нет.
    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Cosmetic", meta = (ClampMin = "1.0"))
    float PenetrationExitTraceMaxDepth = 200.f;

    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Cosmetic")
    float SoundNearThreshold = 5000.f;          // см, ближе — звук без задержки

    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Cosmetic")
    float SpeedOfSoundCmPerSec = 34300.f;

    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Cosmetic")
    TSoftObjectPtr<UBallisticImpactFXLibrary> FXLibrary;

    // Niagara-система billboard-трейсеров. Не назначена — трейсеры выключены.
    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Tracers")
    TSoftObjectPtr<UNiagaraSystem> TracerNiagaraSystem;

    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Tracers", meta = (ClampMin = "0"))
    int32 MaxTracerBufferSize = 100000;

    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Tracers")
    bool bTracersEnabled = false;

    // Включить ribbon-трейлы (независимы от billboard-трейсеров).
    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Trail Ribbon")
    bool bTrailRibbonEnabled = false;

    // Макс. число одновременно отрисовываемых ribbon-трейлов.
    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Trail Ribbon",
        meta = (EditCondition = "bTrailRibbonEnabled", ClampMin = "1", ClampMax = "8192"))
    int32 MaxTrailSlots = 512;

    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Trail Ribbon",
        meta = (EditCondition = "bTrailRibbonEnabled", ClampMin = "2", ClampMax = "256"))
    int32 MaxTrailPoints = 16;

    // Дальность видимости трейлов от камеры (см, FOV-aware).
    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Trail Ribbon",
        meta = (EditCondition = "bTrailRibbonEnabled", ClampMin = "0.0"))
    float TrailCullDistance = 5000.f;

    // Материал ленты (translucent/additive, читает vertex color).
    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Trail Ribbon",
        meta = (EditCondition = "bTrailRibbonEnabled"))
    TSoftObjectPtr<UMaterialInterface> TrailDefaultMaterial;

    // Полная ширина ленты (см).
    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Trail Ribbon",
        meta = (EditCondition = "bTrailRibbonEnabled", ClampMin = "0.01"))
    float TrailDefaultWidth = 20.f;

    // Минимальный путь между точками истории (см). 0 = точка каждый кадр.
    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Trail Ribbon",
        meta = (EditCondition = "bTrailRibbonEnabled", ClampMin = "0.0"))
    float TrailMinPointDistance = 5.f;

    // Вес энергии снаряда в приоритете вытеснения трейлов при переполнении.
    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Trail Ribbon",
        meta = (EditCondition = "bTrailRibbonEnabled", ClampMin = "0.0"))
    float TrailEnergyWeight = 1.f;

    UPROPERTY(EditAnywhere, config, Category = "Ballistic|Trail Ribbon",
        meta = (EditCondition = "bTrailRibbonEnabled", ClampMin = "1.0"))
    float TrailInstigatorBoost = 10.f;
};

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Damage Routing"))
class BALLISTICCORE_API UBallisticDamageRoutingSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    virtual FName GetCategoryName() const override { return FName(BALLISTIC_SETTINGS_CATEGORY); }

    UBallisticDamageRoutingSettings()
    {
        // D2: живой Config/DefaultGame.ini на момент экстракции.
        bFallbackToPointDamage = false;
    }

    // Актор без Target-компонента всё равно получает урон через ApplyPointDamage
    // («урон из коробки»). Выключить для строгого контроля.
    UPROPERTY(EditAnywhere, config, Category = "Ballistics|Damage")
    bool bFallbackToPointDamage = true;
};
