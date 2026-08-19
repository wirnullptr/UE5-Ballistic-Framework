#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BallisticCosmeticStatics.generated.h"

class UNiagaraSystem;

// UBallisticCosmeticStatics

// UBallisticCosmeticSubsystem.
UCLASS()
class BALLISTICCORE_API UBallisticCosmeticStatics : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Вызывать рядом со SpawnBallisticUnit. Дедуп по (Instigator + Location ~5см)
    // за кадр; списывает один общий FX-токен подсистемы.
    UFUNCTION(BlueprintCallable, Category = "Ballistic|Cosmetic", meta = (WorldContext = "WorldContextObject"))
    static void PlayMuzzleFlash(UObject* WorldContextObject, FVector Location, FRotator Rotation,
                               UNiagaraSystem* FlashFX, AActor* Instigator);
};
