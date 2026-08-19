#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "BallisticEventTypes.h"
#include "Data/BallisticImpactFXLibrary.h"
#include "BallisticCosmeticSubsystem.generated.h"

class UDecalComponent;
class USoundBase;
class APawn;
class UNiagaraSystem;
class UNiagaraComponent;
class UMaterialInterface;
class UBallisticTrailRendererComponent;

// Мировая косметика попаданий: impact-Niagara, звук (с задержкой по скорости звука), декали.
UCLASS()
class BALLISTICCORE_API UBallisticCosmeticSubsystem
    : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    // Косметики нет на dedicated-сервере (§8.3).
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // Tick — обработка отложенной звук-очереди + refill токенов по реальному времени.
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override { return bSubscribed; }

    // ── API для muzzle-статик-либы ──────────────────────────────────────────────
    // Списать один FX-токен (общий бюджет). true — токен был и потрачен.
    bool TrySpendFXToken();
    // Дедуп muzzle-flash: false — дубль в этом game-tick (тот же Instigator+позиция).
    bool RegisterMuzzleFlash(const AActor* Instigator, const FVector& Location);

    // ── Конфиг-члены (runtime, из UBallisticCosmeticSettings в OnWorldBeginPlay) ──
    // Прежние config-UPROPERTY — теперь обычные runtime-члены. Источник правды —
    // UBallisticCosmeticSettings (Project Settings). Дефолты = дефолты Settings.
    float FXCullDistance = 5000.f;              // см, при FOV=90
    int32 MaxFXPerSecond = 64;                  // token-bucket по реальному времени
    int32 MaxDecalsInWorld = 500;
    float DecalMaxVisibilityDistance = 5000.f;  // см; дальше камеры — SetVisibility(false). 0 = без лимита
    float PenetrationExitTraceMaxDepth = 200.f; // см; глубина reverse-трейса exit-точки пробития
    float SoundNearThreshold = 5000.f;          // см, ближе — звук без задержки
    float SpeedOfSoundCmPerSec = 34300.f;
    TSoftObjectPtr<UBallisticImpactFXLibrary> FXLibrary;

    // ── Трейсеры (GPU-Niagara мост, Веха 3) ──────────────────────────────────────
    // Один GPU-driven Niagara System читает позиции ВСЕХ активных снарядов разом из
    // SoA через Data Interface Array (НЕ per-projectile CPU-компоненты). Стиль —
    // свойство ассета. TSoftObjectPtr (не hard) — из config-ini, резолв
    // LoadSynchronous() при спавне компонента. Не назначен/выкл -> ноль оверхеда.
    TSoftObjectPtr<UNiagaraSystem> TracerNiagaraSystem;
    int32 MaxTracerBufferSize = 100000;         // потолок буфера, защита памяти
    bool  bTracersEnabled = false;              // default OFF

private:
    void HandleImpactBatch(const TArray<FBallisticImpactEventLite>& Impacts);
    void RefillTokens();               // по реальному (unscaled) времени
    void CacheCameraView();            // FOV + позиция камеры + локальный Pawn, раз в кадр
    float ComputePriority(const FBallisticImpactEventLite& Impact, float DistToCamera) const;
    const FBallisticImpactFXEntry* ResolveFXEntry(EPhysicalSurface Surface) const;
    void SpawnImpactFX(const FBallisticImpactEventLite& Impact, const FBallisticImpactFXEntry& Entry, float Priority);
    void SpawnDecalPooled(const FBallisticImpactEventLite& Impact, const FBallisticImpactFXEntry& Entry, float Priority);
    // Звук с дистанционной задержкой (near→сразу, far→deferred). Общий для входа/выхода.
    void PlayImpactSound(USoundBase* Sound, const FVector& Loc);
    // Низкоуровневая вставка декали в приоритетный пул (SetFadeScreenSize + reuse/evict).
    // Общая точка для обычной/рикошет/exit-декали — вызывающий задаёт материал/размер/ротацию.
    void SpawnDecalIntoPool(UMaterialInterface* Material, const FVector& DecalSize,
                            const FVector& Loc, const FRotator& Rot, float Lifetime, float Priority);
    // Exit-FX пробития: reverse-трейс по пробитому компоненту -> FX/звук/декаль на выходе.
    // Гейт (тип==Penetration + хотя бы одно Penetration FX-поле) — внутри, до трейса.
    void SpawnPenetrationExitFX(const FBallisticImpactEventLite& Impact, const FBallisticImpactFXEntry& Entry, float Priority);
    void ProcessDeferredSounds();
    // Раз в кадр: гасит/показывает пул декалей по дистанции до камеры (глобальная cm-политика).
    void UpdateDecalVisibility();

    FDelegateHandle ImpactBatchHandle;
    bool bSubscribed = false;

    const UBallisticImpactFXLibrary* CachedFXLibrary = nullptr;

    // Кэш камеры (раз в кадр).
    FVector CachedCameraPos = FVector::ZeroVector;
    float   CachedCameraFOV = 90.f;
    TWeakObjectPtr<APawn> CachedLocalPawn;

    // Token bucket — по реальному времени.
    float  FXTokens = 0.f;
    double LastRefillRealTime = -1.0;

    // Отложенные звуки (задержка по скорости звука), общий проход в Tick.
    struct FDeferredSound { float TriggerRealTime = 0.f; TObjectPtr<USoundBase> Sound = nullptr; FVector Location = FVector::ZeroVector; };
    TArray<FDeferredSound> DeferredSounds;

    // Пул декалей — перезапись по НИЗШЕМУ приоритету, не FIFO.
    struct FDecalRecord { TWeakObjectPtr<UDecalComponent> Decal; float Priority = 0.f; };
    TArray<FDecalRecord> DecalPool;

    // Дедуп muzzle-flash в пределах одного game-tick.
    struct FMuzzleKey { TWeakObjectPtr<const AActor> Instigator; FVector Location = FVector::ZeroVector; };
    TArray<FMuzzleKey> MuzzleThisFrame;
    uint64 MuzzleFrameCounter = 0;

    // Scratch — кандидаты FX (индекс + приоритет), переиспользуется, без per-frame аллокаций.
    struct FFXCandidate { int32 Index = 0; float Priority = 0.f; float DistToCamera = 0.f; };
    TArray<FFXCandidate> ScratchCandidates;

    // ── Трейсеры ─────────────────────────────────────────────────────────────────
    // Раз в кадр: снапшот активных не-Dormant позиций из ядра -> Niagara DI Array.
    void UpdateTracers();

    // Персистентный GPU-компонент — спавнится один раз в OnWorldBeginPlay (если
    // фича включена и ассет назначен), живёт до Deinitialize, НЕ пересоздаётся.
    UPROPERTY()
    TObjectPtr<UNiagaraComponent> TracerComponent = nullptr;

    // Scratch под снапшот позиций — переиспользуется, держит ёмкость (ноль per-frame
    // аллокаций в стационарном режиме).
    TArray<FVector> TracerPositionScratch;

    // Scratch под снапшот скоростей (velocity-aligned трейсеры) — тот же принцип,
    // индексно выровнен с TracerPositionScratch (обе копии подряд в UpdateTracers).
    TArray<FVector> TracerVelocityScratch;

    // ── Ribbon-трейлы v2.0 (Шаг 3: пул слотов + кольцевой буфер точек) ────────────
    // Аддитивная ветка, ПАРАЛЛЕЛЬНАЯ billboard-трейсерам выше. Рендер — Шаг 4.

    // Конфиг-члены (runtime, из UBallisticCosmeticSettings в OnWorldBeginPlay).
    bool  bTrailRibbonEnabled = false;
    int32 MaxTrailSlots       = 512;
    int32 MaxTrailPoints      = 16;
    float TrailCullDistance   = 5000.f;
    float TrailMinPointDistance = 5.f;    // 4c: мин. путь между точками истории (см)
    float TrailEnergyWeight   = 1.f;      // 4b: вес энергии в приоритете вытеснения
    float TrailInstigatorBoost = 10.f;    // 4b: множитель приоритета для выстрела локального игрока

    // Слот истории одного снаряда. Точки лежат отдельно в TrailPointBuffer (плоский).
    struct FBallisticTrailSlot
    {
        int32  SparseIndex = INDEX_NONE;  // привязка к снаряду (ядро)
        uint32 Generation  = 0;           // отсев переиспользования sparse чужим снарядом
        uint16 WriteCount  = 0;           // счётчик записей → ring-позиция и число валидных точек
        uint16 ProfileIndex = 0;          // 4a: для резолва per-profile стиля при отправке
        float  Priority    = 0.f;         // live-приоритет (для вытеснения при переполнении)
        bool   bActiveThisFrame = false;  // отметка прохода этого кадра → освобождение неактивных
    };

    // 4a: резолвнутый per-profile стиль ленты, кэш по ProfileIndex (строится один раз).
    struct FBallisticTrailStyle { FLinearColor Color = FLinearColor::White; float WidthMultiplier = 1.f; };
    TArray<FBallisticTrailStyle> TrailStyleByProfile;

    // Кандидат для отбора топ-MaxTrailSlots: Index — в снапшот-массивы TrailData*,
    // Priority — предвычисленный live-приоритет. Scratch, переиспользуется.
    struct FTrailCandidate { int32 Index = 0; float Priority = 0.f; };
    TArray<FTrailCandidate> TrailCandidateScratch;

    // Пул фиксированного размера MaxTrailSlots, аллоцируется один раз (InitTrailPool).
    TArray<FBallisticTrailSlot> TrailSlots;
    // SparseIndex -> SlotIndex. До MaxTrailSlots записей одновременно, не растёт безгранично.
    TMap<int32, int32>          SlotBySparse;
    // Стек свободных слотов.
    TArray<int32>               FreeSlotIndices;
    // Плоский кольцевой буфер точек: [SlotIndex * MaxTrailPoints + (WriteCount % MaxTrailPoints)].
    // Размер MaxTrailSlots * MaxTrailPoints, аллоцируется один раз. Запись по индексу, без Add в Tick.
    TArray<FVector>             TrailPointBuffer;

    // Scratch под выход CopyActiveTracerData (переиспользуются, ноль per-frame аллокаций).
    TArray<FVector>  TrailDataPositions;
    TArray<int32>    TrailDataSparse;
    TArray<uint32>   TrailDataGenerations;
    TArray<uint16>   TrailDataProfiles;                 // 4a: profile для стиля
    TArray<float>    TrailDataEnergies;                 // 4b: энергия для приоритета
    TArray<TWeakObjectPtr<AActor>> TrailDataOwners;     // 4b: owner для instigator-буста

    // Один раз: аллокация TrailSlots/TrailPointBuffer/FreeSlotIndices под текущие капы.
    void InitTrailPool();
    // Один раз: кэш per-profile стилей ленты из UBallisticProjectileData (4a).
    void BuildTrailStyleCache();
    // Раз в кадр (из Tick, за флагом bTrailRibbonEnabled): обновление пула + буфера точек.
    void UpdateTrailRibbon();
    // Запись точки в кольцевой буфер слота + продвижение WriteCount (с защитой от uint16-wrap).
    void AppendTrailPoint(int32 SlotIndex, const FVector& Pos);
    // Раз в кадр после UpdateTrailRibbon: компактный снапшот активных слотов -> рендер-компонент.
    void SendTrailRenderData();

    // ── Рендер ленты (Шаг 4) ─────────────────────────────────────────────────────
    // Runtime-настройки из UBallisticCosmeticSettings.
    TSoftObjectPtr<UMaterialInterface> TrailDefaultMaterial;
    float TrailDefaultWidth = 20.f;

    // Один персистентный компонент-рендер на ВСЕ ленты. Создаётся под bTrailRibbonEnabled,
    // живёт до Deinitialize. UPROPERTY — держит от GC.
    UPROPERTY()
    TObjectPtr<UBallisticTrailRendererComponent> TrailRenderComponent = nullptr;
};
