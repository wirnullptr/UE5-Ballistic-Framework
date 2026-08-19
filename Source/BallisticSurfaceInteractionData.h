#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PhysicalMaterials/PhysicalMaterial.h"   // EPhysicalSurface
#include "Templates/SubclassOf.h"
#include "BallisticSurfaceInteractionData.generated.h"

// Как поверхность реагирует на попадание: порог рикошета, удержание энергии на
// отскоке, пробиваемость. Ключуется по EPhysicalSurface.
USTRUCT(BlueprintType)
struct FBallisticSurfaceInteractionProfile
{
    GENERATED_BODY()

    // Порог угла ОТ ПЛОСКОСТИ поверхности: если grazing-угол меньше — рикошет.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistic|Surface",
        meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float CriticalRicochetAngleDegrees = 15.f;

    //~ Рикошет: раздельное гашение касательной/нормальной составляющих скорости —
    //~ нормаль гасится сильнее, снаряд стелется низко (не «подпрыгивает мячиком»).

    // Удержание СКОРОСТИ вдоль плоскости поверхности (0..1). Высокое — снаряд
    // сохраняет продольный ход и стелется.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistic|Surface",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float TangentialEnergyRetention = 0.7f;

    // Удержание СКОРОСТИ перпендикулярно поверхности (0..1) — высота отскока.
    // Низкое (≈0.15) держит снаряд у земли, убирает «попрыгунчик».
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistic|Surface",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float NormalEnergyRetention = 0.15f;

    // Минимальная энергия для рикошета. Ниже — застревание (симметрично
    // MinEnergyToPenetrate). Медленный снаряд не рикошетит даже под острым углом.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistic|Surface",
        meta = (ClampMin = "0.0"))
    float MinEnergyToRicochet = 30.f;

    // Базовый шанс рикошета при подходящих угле/энергии (0..1). Масштабируется
    // угловым фактором: у порога критического угла шанс близок к нулю.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistic|Surface",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RicochetChance = 0.75f;

    // Случайный разброс направления отражения (градусы) — рикошет не идеально
    // зеркальный. Детерминирован (FRandomStream с seed).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistic|Surface",
        meta = (ClampMin = "0.0", ClampMax = "45.0"))
    float RicochetDirectionJitterDegrees = 4.f;

    // Можно ли пробить эту поверхность насквозь.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistic|Surface")
    bool bPenetrable = false;

    // Доля потери энергии при пробитии (применяется к СКОРОСТИ).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistic|Surface",
        meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PenetrationEnergyLossFraction = 0.4f;

    // Минимальная энергия (после потери) для пробития. Ниже — застревание.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistic|Surface",
        meta = (ClampMin = "0.0"))
    float MinEnergyToPenetrate = 50.f;
};

// UBallisticSurfaceInteractionData
//
// Data-driven карта EPhysicalSurface -> профиль взаимодействия. Дизайнер
// заполняет в редакторе; ассет назначается в UBallisticFramework. Не назначен ->
// рикошет/пробитие выключены (снаряд уничтожается при попадании, как раньше).
UCLASS(BlueprintType)
class BALLISTICCORE_API UBallisticSurfaceInteractionData : public UDataAsset
{
    GENERATED_BODY()

public:
    // Ключ — EPhysicalSurface из PhysMaterial задетой геометрии.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ballistic|Surface")
    TMap<TEnumAsByte<EPhysicalSurface>, FBallisticSurfaceInteractionProfile> SurfaceProfiles;

    // Для поверхностей без явной записи в карте (в т.ч. SurfaceType_Default).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ballistic|Surface")
    FBallisticSurfaceInteractionProfile DefaultProfile;
};
