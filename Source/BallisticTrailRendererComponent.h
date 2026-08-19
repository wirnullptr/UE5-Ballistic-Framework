#pragma once

#include "CoreMinimal.h"
#include "Components/PrimitiveComponent.h"
#include "BallisticTrailRendererComponent.generated.h"

class UMaterialInterface;

// FBallisticTrailRenderSlot — компактный снапшот одного трейла для передачи
// game→render thread. Точки в МИРОВЫХ координатах, порядок голова(свежая)→хвост
// (старая). TInlineAllocator: типичный трейл (≤ MaxTrailPoints) без heap-аллокаций.
struct FBallisticTrailRenderSlot
{
    TArray<FVector, TInlineAllocator<32>> Points;
    uint16 ProfileIndex = 0;                    // информационно (стиль уже резолвнут ниже)
    // Per-profile стиль, резолвнутый на game thread (render thread не читает UObject-профиль).
    FLinearColor Color = FLinearColor::White;   // модулирует vertex color ленты
    float WidthMultiplier = 1.f;                // множитель полуширины
};

// Батч-рендер ribbon-трейлов: один компонент на все трейлы (не per-projectile).
UCLASS()
class BALLISTICCORE_API UBallisticTrailRendererComponent : public UPrimitiveComponent
{
    GENERATED_BODY()

public:
    UBallisticTrailRendererComponent();

    // Полуширина ленты (см) — из TrailDefaultWidth/2.
    void SetTrailHalfWidth(float InHalfWidth) { TrailHalfWidth = FMath::Max(InHalfWidth, 0.01f); }
    float GetTrailHalfWidth() const { return TrailHalfWidth; }

    void SetTrailMaterial(UMaterialInterface* InMaterial);
    UMaterialInterface* GetTrailMaterial() const { return TrailMaterial; }

    // Game thread: принять снапшот активных трейлов и передать в proxy (render command).
    void UpdateTrailRenderData(TArray<FBallisticTrailRenderSlot>&& InSlots);

    //~ UPrimitiveComponent
    virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
    virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
    virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const override;
    virtual int32 GetNumMaterials() const override { return 1; }
    virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
    virtual void SetMaterial(int32 ElementIndex, UMaterialInterface* InMaterial) override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInterface> TrailMaterial = nullptr;

    float TrailHalfWidth = 10.f;

    // Большой фикс-bounds (см) — лента не пропадает при повороте камеры. Простое
    // v2.0-решение (per-frame recalc из экстентов точек — возможный follow-up).
    float BoundsRadius = 1.0e6f;
};
