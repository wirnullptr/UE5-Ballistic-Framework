#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Templates/SubclassOf.h"
#include "BallisticGASDamageConfig.generated.h"

class UGameplayEffect;

// FBallisticGASDamageEntry
//
// Одна запись маппинга DamageProfileTag -> как наносить урон через GAS:
// какой GameplayEffect применить и в какой SetByCaller-тег положить магнитуду.
USTRUCT(BlueprintType)
struct FBallisticGASDamageEntry
{
    GENERATED_BODY()

    // GE, применяемый к цели (обычно instant-effect с execution/modifier по здоровью).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ballistics|GAS")
    TSubclassOf<UGameplayEffect> DamageEffect;

    // Тег SetByCaller, в который мост кладёт Impact.DamageMagnitude.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ballistics|GAS")
    FGameplayTag SetByCallerTag;
};

// UBallisticGASDamageConfig
//
// Data-driven маппинг тегов профиля урона в GAS-эффекты. Дизайнер заполняет
// таблицу в редакторе — код не трогается. Ассет передаётся в
// UBallisticGASDamageStatics::ApplyBallisticDamageViaGAS(Impact, Target, Config).
UCLASS()
class BALLISTICGAS_API UBallisticGASDamageConfig : public UDataAsset
{
    GENERATED_BODY()

public:
    // Ключ — DamageProfileTag из FBallisticImpactEventLite; значение — что применить.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ballistics|GAS")
    TMap<FGameplayTag, FBallisticGASDamageEntry> DamageProfileMap;
};
