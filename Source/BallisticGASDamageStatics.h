#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BallisticEventTypes.h"   // FBallisticImpactEventLite
#include "BallisticGASDamageStatics.generated.h"

class AActor;
class UAbilitySystemComponent;
class UBallisticGASDamageConfig;

// TODO (§9 / И-7): этот мост намеренно вынесен в ОТДЕЛЬНЫЙ модуль BallisticGAS,
// чтобы ядро BallisticCore не линковало GameplayAbilities (гейт И-2/И-7 целевой
// архитектуры). Не добавлять GAS-зависимости в Core; типы ядра использовать только
// через публичные заголовки BallisticCore. Контекст решения — §9
// ballistic_current_state_4.md.

// GAS-инструменты для разработчика: функции для нанесения баллистического урона
// через GAS, вызываются вручную из обработчика OnBallisticDamageReceived.
UCLASS()
class BALLISTICGAS_API UBallisticGASDamageStatics : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Резолв AbilitySystemComponent актора (AI-safe, через UAbilitySystemGlobals). null-safe.
    UFUNCTION(BlueprintCallable, Category = "Ballistic|GAS")
    static UAbilitySystemComponent* ResolveASC(AActor* Actor);

    // Применяет баллистический урон через GAS: резолв Target/Source ASC (И-10),
    // маппинг Impact.DamageProfileTag через Config, MakeOutgoingSpec + SetByCaller
    // (= Impact.DamageMagnitude) + ApplyGameplayEffectSpecToTarget.
    // Возвращает true при успешном применении; false — нет Target ASC / пустой тег /
    // нет записи в конфиге / невалидный spec (без лога — вызывающий решает сам).
    UFUNCTION(BlueprintCallable, Category = "Ballistic|GAS")
    static bool ApplyBallisticDamageViaGAS(
        const FBallisticImpactEventLite& Impact,
        AActor* TargetActor,
        UBallisticGASDamageConfig* Config);
};
