#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BallisticEventTypes.h"
#include "BallisticDamageReceiver.generated.h"

// IBallisticDamageReceiver
//
// Контракт между ядром (damage-agnostic, И-8) и любой damage-системой.
// Реализуют РАЗНЫЕ мосты — GAS, simple ApplyPointDamage, сторонний — ядро о них
// не знает ничего, только раздаёт батч импактов через этот интерфейс.
// Метод чисто-виртуальный (native): мосты — C++ компоненты, регистрируются в
// ядре и получают батч раз в кадр.
UINTERFACE(MinimalAPI, BlueprintType)
class UBallisticDamageReceiver : public UInterface
{
    GENERATED_BODY()
};

class IBallisticDamageReceiver
{
    GENERATED_BODY()

public:
    // Вызывается ядром раз в кадр с батчем всех импактов. Реализация решает,
    // как превратить их в урон (GE, ApplyPointDamage, что угодно).
    virtual void ApplyBallisticImpacts(const TArray<FBallisticImpactEventLite>& Impacts) = 0;
};
