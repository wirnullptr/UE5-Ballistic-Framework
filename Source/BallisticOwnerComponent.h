#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BallisticEventTypes.h"   // FBallisticImpactEventLite, FOnBallisticImpactBatch
#include "BallisticOwnerComponent.generated.h"

// Фильтрует импакты по владельцу «из коробки»: вешается на актора-стрелка (снаряды
// со Owner = он сам), биндишься на OnMyProjectileImpact — получаешь только свои импакты.
// Это FEEDBACK-канал (хитмаркеры / UI / звук стрелку), НЕ damage-путь.
UCLASS(ClassGroup=(Ballistic), meta=(BlueprintSpawnableComponent))
class BALLISTICCORE_API UBallisticOwnerComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBallisticOwnerComponent();

    // Батч импактов только от снарядов этого актора.
    UPROPERTY(BlueprintAssignable, Category = "Ballistics|Events")
    FOnBallisticImpactBatch OnMyProjectileImpact;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    // Обработчик native-батча ядра. Фильтрует по владельцу, ре-broadcast'ит своё.
    void HandleImpactBatch(const TArray<FBallisticImpactEventLite>& Impacts);

    // Хэндл подписки на native-делегат ядра — обязателен для отписки в EndPlay
    // (иначе dangling-подписка на dead actor).
    FDelegateHandle ImpactBatchHandle;

    // Переиспользуемый буфер под отфильтрованный батч — без per-frame реаллокаций.
    TArray<FBallisticImpactEventLite> OwnedImpactsScratch;
};
