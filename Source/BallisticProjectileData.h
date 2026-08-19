#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"        // FGameplayTag (И-2: GameplayTags, не GameplayAbilities)
#include "Data/BallisticData.h"          // EBallisticTraceType
#include "BallisticProjectileData.generated.h"

// Статические данные снаряда (масса, скорость, drag, коллизия, рикошет/пробитие).
// Строка таблицы профилей; индекс строки = ProfileIndex.
USTRUCT(BlueprintType)
struct BALLISTICCORE_API FBallisticProjectileProfile
{
    GENERATED_BODY()

    // Отображаемое имя — только для редактора, в рантайме не используется
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
    FName ProfileName = NAME_None;

    // ── Физика ───────────────────────────────────────────────────────────────

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Physics",
        meta = (ClampMin = "0.0001", UIMin = "0.0001",
                ToolTip = "Масса снаряда в кг. Влияет на кинетическую энергию при импакте."))
    float Mass = 0.01f;

    // Явно LinearDrag: модель линейного дрэга dv/dt = G - k*v, совпадает с FBallisticDragSolver.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Physics",
        meta = (ClampMin = "0.0", ClampMax = "1.0",
                ToolTip = "Коэффициент линейного аэродинамического сопротивления [0..1]. 0 = вакуум, 0.01 = пуля, 0.1 = теннисный мяч."))
    float LinearDrag = 0.01f;

    // Gravity — вектор (солвер оперирует вектором; бонус — любое направление гравитации).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Physics",
        meta = (ToolTip = "Вектор ускорения гравитации в см/с². (0,0,-980) = стандарт UE вниз, (0,0,0) = невесомость."))
    FVector Gravity = FVector(0.f, 0.f, -980.f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Physics",
        meta = (ClampMin = "0.0",
                ToolTip = "Начальная скорость снаряда в см/с. 90000 ≈ 900 м/с (типичная пуля)."))
    float Speed = 90000.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Physics",
        meta = (ClampMin = "0.1",
                ToolTip = "Максимальное время жизни в секундах."))
    float MaxLifeTime = 10.f;

    // ── LOD ──────────────────────────────────────────────────────────────────

    // Opt-in под будущий low-freq resolve во сне (coarse-коллизии). Дефолт false =
    // полная заморозка. В этой вехе поле только объявлено, ещё не используется.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "LOD",
        meta = (ToolTip = "Разрешить редкие coarse-коллизии пока снаряд спит (dormant). Дефолт false = полная заморозка."))
    bool bResolveWhileDormant = false;

    // ── Коллизия ─────────────────────────────────────────────────────────────

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision",
        meta = (ClampMin = "0.1",
                ToolTip = "Радиус коллизионной сферы/капсулы в см."))
    float CollisionRadius = 2.f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision",
        meta = (ToolTip = "Тип трассировки. Line = дёшево, Sphere = стандарт, Capsule = для продолговатых."))
    EBallisticTraceType TraceType = EBallisticTraceType::Sphere;

    // Используется только для Capsule. EditCondition скрывает поле в редакторе
    // для Line и Sphere — дизайнер не увидит лишних настроек.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Collision",
        meta = (ClampMin = "0.1",
                EditCondition = "TraceType == EBallisticTraceType::Capsule",
                EditConditionHides,
                ToolTip = "Полудлина капсулы вдоль оси движения в см. Только для Capsule."))
    float CapsuleHalfHeight = 10.f;

    // ── Взаимодействие (рикошет/пробитие, Фаза 3) ─────────────────────────────

    // Общий лимит рикошетов + пробитий. Достигнут -> снаряд уничтожается.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ballistic|Interaction",
        meta = (ClampMin = "0",
                ToolTip = "Макс. число взаимодействий (рикошет+пробитие) до уничтожения."))
    int32 MaxInteractions = 3;

    // Может ли рикошет вернуться в стрелка. Default false: на свежем выстреле
    // владелец игнорируется всегда; после первого interaction — по этому флагу.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ballistic|Interaction",
        meta = (ToolTip = "Разрешить рикошет обратно в стрелка (после первого отскока)."))
    bool bCanRicochetHitInstigator = false;

    // Мин. путь между взаимодействиями (см) — отсекает вырожденные микро-отскоки
    // в узких углах (Ловушка 5).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ballistic|Interaction",
        meta = (ClampMin = "0.0",
                ToolTip = "Мин. дистанция между отскоками в см. Ближе — снаряд уничтожается."))
    float MinTravelBetweenInteractions = 10.f;

    // ── Урон ─────────────────────────────────────────────────────────────────

    // Тег ТИПА снаряда (Damage.Rifle, Damage.AP), не конкретного выстрела. Ядро
    // только несёт его в событие (И-2: GameplayTags, не GameplayAbilities); GAS-мост
    // маппит его через UBallisticGASDamageConfig в {GE, SetByCallerTag}. Пустой тег =
    // снаряд без GAS-урона (Simple/fallback работают по DamageMagnitude независимо).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ballistic|Damage",
        meta = (ToolTip = "Тег типа снаряда для маппинга в GAS-эффект. Пустой = без GAS-урона."))
    FGameplayTag DamageProfileTag;

    // ── Ribbon-трейл (косметика; ядро НЕ читает — только косметик-подсистема) ──────
    // Per-profile стиль ленты. Резолвится косметикой по ProfileIndex (Ribbon v2.1).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ballistic|Trail",
        meta = (ToolTip = "Цвет ленты-трейла (модулирует vertex color). Косметика, ядро не использует."))
    FLinearColor TrailColor = FLinearColor::White;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ballistic|Trail",
        meta = (ClampMin = "0.0",
                ToolTip = "Множитель ширины ленты-трейла относительно глобальной TrailDefaultWidth."))
    float TrailWidthMultiplier = 1.f;
};


// UBallisticProjectileData
//
// Primary DataAsset. Один на проект (или на геймплейный контекст).
// Дизайнер открывает его в редакторе и заполняет массив Profiles.
// Индекс элемента в массиве = ProfileIndex, который хранится в SoA.
//
// Важно: DataAsset загружается через TSoftObjectPtr и резолвится при старте
// SubSystem (или при первом спауне). После резолва — только чтение,
// никаких аллокаций в хот-пасе.
UCLASS(BlueprintType)
class BALLISTICCORE_API UBallisticProjectileData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    // Массив профилей. Индекс = ProfileIndex в SoA.
    // Дизайнер добавляет строки здесь — не плодит отдельные ассеты.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Profiles",
        meta = (ToolTip = "Список профилей снарядов. Индекс строки = ProfileIndex при спауне."))
    TArray<FBallisticProjectileProfile> Profiles;

    // ── Валидация ─────────────────────────────────────────────────────────────

    // Проверка индекса — вызывай перед SpawnBallisticUnit если не уверен.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ballistic|Data")
    bool IsValidProfileIndex(int32 Index) const
    {
        return Profiles.IsValidIndex(Index);
    }

    // Возвращает профиль по индексу. Проверяй IsValidProfileIndex заранее —
    // здесь нет ручного рейнджа, только ensure в Debug.
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ballistic|Data")
    const FBallisticProjectileProfile& GetProfile(int32 Index) const
    {
        ensure(Profiles.IsValidIndex(Index));
        return Profiles[Index];
    }

#if WITH_EDITOR
    // PostEditChangeProperty — валидируем данные сразу в редакторе,
    // а не на рантайме когда снаряды уже летят не туда.
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
