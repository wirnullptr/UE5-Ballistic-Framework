#pragma once

#include "CoreMinimal.h"

// Closed-form решение линейного дрэга dv/dt = G - k*v. Всё static, без
// движковых зависимостей кроме FVector/FMath — юнит-тестируется без движка.
// Фундамент вехи 1 (dormant wake-up sweep); на вехе 0 ещё не вызывается.
struct FBallisticDragSolver
{
    static FVector SampleVelocity(const FVector& V0, const FVector& G, float K, float T);
    static FVector SamplePosition(const FVector& X0, const FVector& V0, const FVector& G, float K, float T);
    static int32   ComputeWakeSegmentCount(const FVector& G, float TotalTime, float SagTolerance, int32 MaxSegments);
};
