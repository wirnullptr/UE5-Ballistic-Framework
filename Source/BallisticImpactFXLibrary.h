#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PhysicalMaterials/PhysicalMaterial.h"   // EPhysicalSurface
#include "BallisticImpactFXLibrary.generated.h"

class UNiagaraSystem;
class USoundBase;
class UMaterialInterface;

// FBallisticImpactFXEntry — косметика попадания по конкретной поверхности.
USTRUCT(BlueprintType)
struct FBallisticImpactFXEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistic|FX")
    TObjectPtr<UNiagaraSystem> ImpactNiagara = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistic|FX")
    TObjectPtr<USoundBase> ImpactSound = nullptr;

    // Опциональные FX по типу взаимодействия. Не задано — фоллбек на ImpactNiagara/ImpactSound.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistic|FX")
    TObjectPtr<UNiagaraSystem> RicochetNiagara = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistic|FX")
    TObjectPtr<USoundBase> RicochetSound = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistic|FX")
    TObjectPtr<UNiagaraSystem> PenetrationNiagara = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistic|FX")
    TObjectPtr<USoundBase> PenetrationSound = nullptr;

    // Материал декали по типу взаимодействия (опционально; фоллбек на DecalMaterial).
    // Напр. скол/царапина для рикошета, дырка для пробития.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistic|FX")
    TObjectPtr<UMaterialInterface> RicochetDecalMaterial = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistic|FX")
    TObjectPtr<UMaterialInterface> PenetrationDecalMaterial = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistic|FX")
    TObjectPtr<UMaterialInterface> DecalMaterial = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistic|FX")
    FVector DecalSize = FVector(4.f);

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistic|FX")
    float DecalLifetime = 30.f;
};

// UBallisticImpactFXLibrary — карта EPhysicalSurface -> FX/звук/декаль.
// МИРОВАЯ косметика (видна всем), не путать с личным фидбеком UBallisticOwnerComponent.
UCLASS(BlueprintType)
class BALLISTICCORE_API UBallisticImpactFXLibrary : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ballistic|FX")
    TMap<TEnumAsByte<EPhysicalSurface>, FBallisticImpactFXEntry> SurfaceFX;

    // Для поверхностей без явной записи (в т.ч. SurfaceType_Default).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ballistic|FX")
    FBallisticImpactFXEntry DefaultFX;
};
