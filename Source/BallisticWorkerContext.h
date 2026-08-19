#pragma once

#include "CoreMinimal.h"

// Per-batch scratch space для ParallelFor-прохода (И-1). Каждый батч пишет
// только в свой контекст — общие структуры трогает только serial-merge после прохода.
struct FBallisticWorkerContext
{
#if !UE_BUILD_SHIPPING
    TArray<TPair<int32, FVector>> TrailPoints; // trail сюда, не в SoA напрямую (И-1)
#endif

    void Reset()
    {
#if !UE_BUILD_SHIPPING
        TrailPoints.Reset(); // держим ёмкость
#endif
    }
};
