#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BallisticDamageTarget.h"
#include "BallisticEventTypes.h"   // FBallisticImpactEventLite, FOnBallisticDamageReceived
#include "BallisticDamageTargetComponent.generated.h"

// Уведомляющий приёмник баллистических импактов: регистрируется в routing-подсистеме
// и на каждый импакт броадкастит OnBallisticDamageReceived. Применение урона решает
// разработчик в обработчике (автоприменения по умолчанию нет).
UCLASS(ClassGroup=(Ballistic), meta=(BlueprintSpawnableComponent))
class BALLISTICCORE_API UBallisticDamageTargetComponent
    : public UActorComponent, public IBallisticDamageTarget
{
    GENERATED_BODY()

public:
    UBallisticDamageTargetComponent();

    // Броадкаст НА КАЖДЫЙ импакт из суб-батча — реакцию решает разработчик.
    UPROPERTY(BlueprintAssignable, Category = "Ballistic|Damage")
    FOnBallisticDamageReceived OnBallisticDamageReceived;

    // Опционально: ДОПОЛНИТЕЛЬНО зовёт ApplyPointDamage ПОСЛЕ broadcast (для тех,
    // кто хочет «совсем без кода»). Default false — по умолчанию ничего не
    // применяется автоматически.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ballistic|Damage")
    bool bAutoApplyPointDamage = false;

    // IBallisticDamageTarget — чисто-виртуальный native, плоский override (без _Implementation).
    virtual void ReceiveBallisticImpacts(const TArray<FBallisticImpactEventLite>& Impacts) override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
