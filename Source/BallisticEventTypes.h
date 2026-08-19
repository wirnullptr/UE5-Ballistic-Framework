#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/NetSerialization.h"
#include "PhysicalMaterials/PhysicalMaterial.h"   // EPhysicalSurface
#include "BallisticEventTypes.generated.h"

class AActor;
class UPrimitiveComponent;

// EBallisticImpactType — характер взаимодействия снаряда с поверхностью на импакте.
// Заполняется ядром (система рикошета/пробития знает исход) для косметики: разные
// FX/звук/декаль на обычное попадание / рикошет / пробитие.
UENUM(BlueprintType)
enum class EBallisticImpactType : uint8
{
    Normal,
    Ricochet,
    Penetration
};

// Компактное (net-quantized) событие импакта для damage-каналов и косметики.
USTRUCT(BlueprintType)
struct BALLISTICCORE_API FBallisticImpactEventLite
{
    GENERATED_BODY()

    // Идентичность снаряда = хэндл (sparse index + generation). Разбит на два
    // поля, т.к. generation — uint32, а он в BP не представим.
    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    int32 ProjectileIndex = INDEX_NONE;

    // uint32 не поддерживается Blueprint'ом — reflected, но не BlueprintReadOnly
    // (тот же приём, что в FBallisticHandle::Generation).
    UPROPERTY()
    uint32 ProjectileGeneration = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    FVector_NetQuantize ImpactPoint = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    FVector_NetQuantizeNormal ImpactNormal = FVector::UpVector;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    TWeakObjectPtr<AActor> HitActor;

    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    TWeakObjectPtr<UPrimitiveComponent> HitComponent;

    // Кость попадания (для zone-damage / хедшотов).
    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    FName HitBoneName = NAME_None;

    // Владелец снаряда — источник урона (instigator) для damage-мостов.
    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    TWeakObjectPtr<AActor> Instigator;

    // Тег профиля урона — маппится мостом в GE/множитель.
    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    FGameplayTag DamageProfileTag;

    // Магнитуда урона для damage-мостов.
    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    float DamageMagnitude = 0.f;

    // Скорость снаряда в момент попадания (см/с).
    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    float SpeedAtImpact = 0.f;

    // Тип поверхности (из PhysMaterial) — для FX/звука/декали.
    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    TEnumAsByte<EPhysicalSurface> SurfaceType = SurfaceType_Default;

    // Нормализованное направление скорости снаряда в момент импакта (net-quantized).
    // Косметика: ориентация декали рикошета, направленные FX.
    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    FVector_NetQuantizeNormal IncomingDirection = FVector::ForwardVector;

    // Характер взаимодействия (обычное/рикошет/пробитие) — для type-specific FX/декали.
    UPROPERTY(BlueprintReadOnly, Category = "Ballistics")
    EBallisticImpactType ImpactType = EBallisticImpactType::Normal;
};

// Делегаты импакт-батча
//
// Native — для C++ подписчиков (cosmetic-сабсистема, мосты). Dynamic — для BP.
// Оба несут ВЕСЬ батч за кадр (не по одному событию) — 1 вызов/кадр, не 100k.
DECLARE_MULTICAST_DELEGATE_OneParam(
    FOnBallisticImpactBatchNative, const TArray<FBallisticImpactEventLite>&);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnBallisticImpactBatch, const TArray<FBallisticImpactEventLite>&, Impacts);

// FOnBallisticDamageReceived
//
// Уведомление цели об импакте. Broadcast НА КАЖДЫЙ импакт (не на батч) — в BP
// удобнее один Impact за раз, чем разворачивать массив (согласовано с Ballistic
// Owner). Компонент только уведомляет; применять урон / любую реакцию решает
// разработчик в своём обработчике.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FOnBallisticDamageReceived, const FBallisticImpactEventLite&, Impact);
