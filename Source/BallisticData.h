#pragma once

#include "CoreMinimal.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Engine/HitResult.h"                   // FHitResult (game-таргет IWYU; в editor подтягивался транзитивно)
#include "BallisticData.generated.h"

// EBallisticTraceType
UENUM(BlueprintType)
enum class EBallisticTraceType : uint8
{
    Line     UMETA(DisplayName = "Line (Fast)"),
    Sphere   UMETA(DisplayName = "Sphere (Balanced)"),
    Capsule  UMETA(DisplayName = "Capsule (Accurate)"),
};

// EBallisticLODTier — тир детализации снаряда по расстоянию до наблюдателя.
// LOD3_Dormant — снаряд заморожен: не интегрируется и не трейсится, живёт
// аналитически по (x0,v0,t0) до пробуждения (retroactive wake-sweep).
UENUM(BlueprintType)
enum class EBallisticLODTier : uint8
{
    LOD0_Full     UMETA(DisplayName = "LOD0 Full"),
    LOD1_Reduced  UMETA(DisplayName = "LOD1 Reduced"),
    LOD2_Coarse   UMETA(DisplayName = "LOD2 Coarse"),
    LOD3_Dormant  UMETA(DisplayName = "LOD3 Dormant"),
};

// FBallisticHandle
USTRUCT(BlueprintType)
struct FBallisticHandle
{
    GENERATED_BODY()

    UPROPERTY()
    int32 Index = INDEX_NONE;

    UPROPERTY()
    uint32 Generation = 0;

    FORCEINLINE bool IsValid() const { return Index != INDEX_NONE; }

    FORCEINLINE bool operator==(const FBallisticHandle& Other) const
    {
        return Index == Other.Index && Generation == Other.Generation;
    }
};

// FBallisticSpawnParams
//
// Остаются только данные специфичные для конкретного экземпляра:
//   позиция, направление, владелец.
//
// Rationale: Mass/Drag/Gravity are identical across all projectiles of one type.
// Storing them 50,000 times in the SoA would waste cache lines. The profile is
// read once at spawn; per-instance state keeps only the 2-byte index.
USTRUCT(BlueprintType)
struct FBallisticSpawnParams
{
    GENERATED_BODY()

    // Индекс профиля в UBallisticProjectileData::Profiles.
    // SubSystem должен иметь загруженный DataAsset до первого SpawnBallisticUnit.
    // uint16: 0..65534 профилей. 65535 = невалидный (INDEX_NONE аналог).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Profile",
        meta = (ToolTip = "Индекс строки в BallisticProjectileData. Определяет физику и коллизию снаряда."))
    uint8 ProfileIndex = 0;

    // ── Данные экземпляра (уникальны для каждого спауна) ─────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
    FVector StartPosition = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
    FVector Direction = FVector::ForwardVector;

    // Владелец — игнорируется при трассировке. TObjectPtr безопасен для GC.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance")
    TObjectPtr<AActor> Owner = nullptr;

    // Опциональный скейл скорости — позволяет дать одному профилю разные
    // начальные скорости (например, усиленный выстрел). 1.0 = значение из профиля.
    // Если дизайнеры не используют — компилятор это знает, накладных расходов нет.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Instance",
        meta = (ClampMin = "0.01", ClampMax = "10.0",
                ToolTip = "Множитель скорости относительно профиля. 1.0 = без изменений."))
    float SpeedMultiplier = 1.f;
};

// FBallisticImpactEvent
USTRUCT(BlueprintType)
struct FBallisticImpactEvent
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    FBallisticHandle ProjectileHandle;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    FVector Location = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    FVector Normal = FVector::UpVector;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    TWeakObjectPtr<AActor> HitActor;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    TWeakObjectPtr<AActor> OwnerActor;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    float ImpactEnergy = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    TEnumAsByte<EPhysicalSurface> SurfaceType = SurfaceType_Default;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    EBallisticTraceType TraceType = EBallisticTraceType::Sphere;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    FHitResult HitResult;

    // ProfileIndex при импакте — полезно для дизайнеров: "эта поверхность пробита
    // снарядом типа 3" без необходимости лезть в SubSystem.
    UPROPERTY(BlueprintReadOnly, Category = "Ballistics",
        meta = (ToolTip = "Индекс профиля снаряда который вызвал этот импакт."))
    uint8 ProfileIndex = 0;
};
