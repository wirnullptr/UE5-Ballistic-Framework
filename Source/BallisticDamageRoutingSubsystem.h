#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/ObjectKey.h"
#include "BallisticDamageReceiver.h"
#include "BallisticDamageTarget.h"
#include "BallisticEventTypes.h"
#include "BallisticDamageRoutingSubsystem.generated.h"

// UBallisticDamageRoutingSubsystem
//
// ЕДИНСТВЕННЫЙ in-box IBallisticDamageReceiver ядра. Получает глобальный батч,
// группирует его по задетому актору за один проход O(M) и раздаёт суб-батчи
// зарегистрированным IBallisticDamageTarget'ам (O(1) hash-lookup на цель).
// Незарегистрированные акторы — опциональный fallback ApplyPointDamage (§7.3:
// урон из коробки без кода покупателя). Подсистема, не компонент — покупателю
// нечего вешать на GameState, ноль ручной настройки.
//
// Масштаб: число вызовов ReceiveBallisticImpacts = число УНИКАЛЬНЫХ задетых
// целей, не импактов × целей (в этом весь смысл против per-target-receiver).
UCLASS()
class BALLISTICCORE_API UBallisticDamageRoutingSubsystem
    : public UWorldSubsystem, public IBallisticDamageReceiver
{
    GENERATED_BODY()

public:
    // Регистрируемся в ядре в OnWorldBeginPlay (НЕ Initialize): порядок инициализации
    // сабсистем в коллекции недетерминирован, а здесь UBallisticFramework гарантированно
    // уже создан. Отписка — в Deinitialize.
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    virtual void Deinitialize() override;

    // IBallisticDamageReceiver — точка входа глобального батча из ядра.
    virtual void ApplyBallisticImpacts(const TArray<FBallisticImpactEventLite>& Batch) override;

    // ── Публичный API для target-компонентов ───────────────────────────────────
    // Одна цель = одна запись. Повторная регистрация того же актора → warning,
    // первая побеждает, вторая отбрасывается (двойной урон невозможен by construction).
    void RegisterDamageTarget(AActor* Owner, TScriptInterface<IBallisticDamageTarget> Target);
    void UnregisterDamageTarget(AActor* Owner);

protected:
    // Runtime-член, наполняется из UBallisticDamageRoutingSettings в OnWorldBeginPlay.
    // Задетый актор без зарегистрированной цели получает урон через ApplyPointDamage.
    bool bFallbackToPointDamage = true;

private:
    // Реестр целей. TObjectKey (не TWeakObjectPtr как ключ) — хеш стабилен даже
    // после GC. НЕ UPROPERTY: TObjectKey не рефлектится; висящие записи чистятся
    // EndPlay'ем цели (штатно) + IsValid при проходе (страховка).
    TMap<TObjectKey<AActor>, TScriptInterface<IBallisticDamageTarget>> DamageTargets;

    // Персистентные scratch-контейнеры — Reset() держит ёмкость, ноль аллокаций
    // в стационаре. Группировка в пределах ОДНОГО вызова: ключ — сырой AActor*
    // (все живые в этом кадре), не нужно resolve'ить TObjectKey обратно.
    TMap<AActor*, TArray<int32>> ImpactsByTargetScratch;
    TArray<FBallisticImpactEventLite> SubBatchScratch;
};
