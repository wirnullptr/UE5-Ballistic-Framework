#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BallisticEventTypes.h"
#include "BallisticDamageTarget.generated.h"

// IBallisticDamageTarget
//
// Контракт актора-ЦЕЛИ, принимающего баллистический урон. В отличие от
// IBallisticDamageReceiver (глобальный приёмник всего батча — им является ТОЛЬКО
// routing-подсистема ядра), цель получает ТОЛЬКО свои импакты — роутер уже
// сгруппировал батч по задетому актору и отфильтровал.
//
// Батч на цель, не вызов на импакт: при 100+ импактах в одну цель — один вызов
// ReceiveBallisticImpacts с суб-батчем, не 100 вызовов.
UINTERFACE(MinimalAPI, BlueprintType)
class UBallisticDamageTarget : public UInterface
{
    GENERATED_BODY()
};

class IBallisticDamageTarget
{
    GENERATED_BODY()

public:
    // Суб-батч импактов ТОЛЬКО по этой цели. Реализация решает, как превратить
    // их в урон (ApplyPointDamage, GAS-эффект, кастомная модель).
    virtual void ReceiveBallisticImpacts(const TArray<FBallisticImpactEventLite>& Impacts) = 0;
};
