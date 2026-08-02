# Ballistic Framework — Документация

## Обзор

Ballistic Framework — плагин для Unreal Engine 5, реализующий высокопроизводительную симуляцию большого количества одновременных снарядов без деградации FPS. Ядро построено на SoA-архитектуре (Structure of Arrays) с sparse/dense-индексацией и generation handle, параллельной обработкой через `ParallelFor`, и адаптивной системой LOD, снижающей стоимость коллизий и косметики по мере удаления от игрока.

Плагин состоит из трёх независимых подсистем:

- **Core** (`UBallisticSubsystem`) — симуляция полёта, коллизии, рикошет и пробитие. GAS-free, damage-agnostic — ядро не знает ни о системе урона, ни о косметике.
- **Cosmetic** (`UBallisticCosmeticSubsystem`) — визуальный и звуковой отклик: декали, частицы, звук, трейсеры.
- **Damage Routing** (`UBallisticDamageRoutingSubsystem`) — маршрутизация урона к целям, опционально интегрируется с GAS через отдельный модуль `BallisticGAS`, без обязательной зависимости.

## Установка

1. Скопируй папку плагина в `[Проект]/Plugins/`
2. `Edit → Plugins` → включи `Ballistic Subsystem Pro`
3. Перезапусти редактор

## Быстрый старт

1. Создай Data Asset `UBallisticProjectileData` — задай хотя бы один профиль снаряда
2. `Project Settings → Ballistic Subsystem Pro → Core` → назначь этот Data Asset в `ProjectileDataAsset`
3. Вызови `SpawnBallisticUnit` (C++ или Blueprint) с индексом нужного профиля
4. Для рикошета/пробития — создай `UBallisticSurfaceInteractionData`, назначь в `SurfaceInteractionAsset`
5. Для визуального отклика — создай `UBallisticImpactFXLibrary`, назначь в `Cosmetic → FXLibrary`

---

## Project Settings

Все настройки — в `Project Settings → Plugins → Ballistic Subsystem Pro`, три раздела.

### Core

| Поле | Дефолт | Описание |
|---|---|---|
| `ProjectileDataAsset` | — | Data Asset с профилями снарядов. Обязателен — без него спавн падает. |
| `SurfaceInteractionAsset` | — | Data Asset взаимодействия с поверхностями. Не назначен → рикошет/пробитие выключены. |
| `ParallelThreshold` | 64 | Порог числа снарядов, с которого включается параллельная обработка. |
| `BatchSize` | 256 | Размер батча для `ParallelFor`. |
| `DormantDistance` | 15000 см | Дистанция ухода снаряда в заморозку (dormant). |
| `WakeSweepSagTolerance` | 5 см | Точность retroactive-проверки при пробуждении из dormant. |
| `MaxWakeSegments` | 16 | Максимум сегментов wake-sweep. |
| `CollisionLOD1Fraction` | 0.33 | Доля `DormantDistance`, ближе которой — полная точность коллизий (L0). |
| `CollisionLOD2Fraction` | 0.66 | Доля `DormantDistance` для перехода L1 → L2 (редкие line-трейсы). |
| `CollisionLOD2ThrottleInterval` | 3 | На L2 — трейс раз в N substep'ов. |
| `PenetrationIgnoreClearDistance` | 100 см | После пробития компонент игнорируется, пока снаряд не отойдёт на эту дистанцию (защита от повторного хита при пробитии толстых стен). |
| `RicochetRandomSeed` | 0x5EED | Seed детерминированного потока рикошета. |
| `MaxUnits` | 100000 | Максимум одновременно живых снарядов — определяет размер SoA-аллокации при старте. |

### Cosmetic

| Поле | Дефолт | Описание |
|---|---|---|
| `FXCullDistance` | 5000 см | Дистанция отсечения FX (FOV-aware). |
| `MaxFXPerSecond` | 64 | Token-bucket лимит FX в реальном времени (независим от масштаба игрового времени — работает и в bullet-time). |
| `MaxDecalsInWorld` | 500 | Кап пула декалей. |
| `DecalMaxVisibilityDistance` | 5000 см | Дальность видимости декали от камеры (движковый механизм `FadeScreenSize` не даёт настроить дальность в см напрямую — плагин реализует собственный per-frame контроль поверх него). |
| `SoundNearThreshold` | 5000 см | Ближе этой дистанции — звук без задержки распространения. |
| `SpeedOfSoundCmPerSec` | 34300 | Скорость звука для расчёта задержки на дальних попаданиях. |
| `FXLibrary` | — | Data Asset с профилями импакт-эффектов по типу поверхности. |
| `TracersEnabled` (`bTracersEnabled`) | false | Билборд-трейсеры (лёгкие точки, GPU-Niagara мост, CPU-симуляция). |
| `TracerNiagaraSystem` | — | Niagara-система для билборд-трейсеров. Не назначена → трейсеры выключены. |
| `MaxTracerBufferSize` | 100000 | Потолок буфера позиций трейсеров. |
| `TrailRibbonEnabled` (`bTrailRibbonEnabled`) | false | Ribbon-трейлы — полноценная 3D-геометрия хвоста, видна с любого ракурса (в т.ч. полёт на камеру, где билборд-спрайты не видны). Независим от билборд-трейсеров, можно включать оба. |
| `MaxTrailSlots` | 512 | Лимит одновременных активных лент (независим от `MaxTracerBufferSize`). |
| `MaxTrailPoints` | 16 | Точек истории на одну ленту. |
| `TrailCullDistance` | 5000 см | Дистанция отсечения лент от камеры. |
| `TrailMinPointDistance` | 5 см | Минимальное расстояние между соседними точками истории — избегает скопления точек у медленных/остановившихся снарядов. |
| `TrailDefaultMaterial` | — | Материал ленты по умолчанию (должен использовать Vertex Color для fade/цвета). |
| `TrailDefaultWidth` | 20 | Базовая ширина ленты. |

### Damage Routing

| Поле | Дефолт | Описание |
|---|---|---|
| `bFallbackToPointDamage` | true | Если задетый актор не зарегистрирован как цель — применяется стандартный `ApplyPointDamage` ("урон из коробки" без ручной настройки). |

---

## Data Assets

### `UBallisticProjectileData` — профили снарядов

Один asset, массив `Profiles` — индекс в массиве = `ProfileIndex` при спавне. Ключевые поля профиля:

- **Physics**: `Mass` (кг), `DragCoefficient` [0-1], `Gravity`, `InitialSpeed` (см/с), `MaxLifetime` (сек)
- **Collision**: `TraceRadius`, `TraceType` (Line/Sphere/Capsule)
- **LOD**: `bResolveWhileDormant` — разрешить редкие коллизии во сне (дефолт false = полная заморозка)
- **Interaction**: `MaxInteractionCount` (лимит рикошетов+пробитий до уничтожения), `bCanRicochetHitInstigator`, минимальная дистанция между отскоками
- **Damage**: `DamageProfileTag` — тег для маппинга в GAS-эффект (пусто = без GAS-урона)
- **Trail Ribbon** (per-profile override): цвет и множитель ширины ленты для этого типа снаряда

### `UBallisticSurfaceInteractionData` — профили поверхностей

`TMap<EPhysicalSurface, FBallisticSurfaceInteractionProfile>` + `DefaultProfile` (фоллбек). Ключевые поля профиля:

- `CriticalRicochetAngleDegrees` (дефолт 15°) — угол, ниже которого рикошет предпочтительнее пробития
- `TangentialEnergyRetention` / `NormalEnergyRetention` — сохранение энергии по касательной/нормали при отскоке
- `MinEnergyToRicochet` / `RicochetChance` / `RicochetDirectionJitterDegrees`
- `bPenetrable`, `PenetrationEnergyLossFraction`, `MinEnergyToPenetrate`

### `UBallisticImpactFXLibrary` — визуальный/звуковой отклик

`TMap<EPhysicalSurface, FBallisticImpactFXEntry>` + `DefaultFX`. Поля записи:

- `ImpactNiagara` / `ImpactSound` / `DecalMaterial` — базовый набор для обычного попадания
- `RicochetNiagara` / `RicochetSound` / `RicochetDecalMaterial` — override для рикошета (фоллбек на базовые, если не заданы)
- `PenetrationNiagara` / `PenetrationSound` / `PenetrationDecalMaterial` — override для пробития (аналогично; если задан хотя бы один — на выходе из объекта тоже спавнится эффект)
- `DecalSize`, `DecalLifetime`

---

## Система урона

Damage Routing спроектирован как **уведомляющий**, не автоприменяющий — по умолчанию система сообщает о попадании, но не наносит урон сама, чтобы исключить двойное применение урона by construction.

### Базовая настройка

1. Добавь `UBallisticDamageTargetComponent` на актор, который должен получать урон
2. Подпишись на `OnBallisticDamageReceived` (BlueprintAssignable-делегат) — броадкастится на каждый импакт из суб-батча
3. В обработчике сам реши, что делать с уроном: применить `ApplyPointDamage`, GAS-эффект, кастомную логику, или проигнорировать

Если не зарегистрировать `UBallisticDamageTargetComponent` — по умолчанию сработает `ApplyPointDamage` (стандартный урон "из коробки"), если `bFallbackToPointDamage = true` в Damage Routing Settings (дефолт).

### Автоматический Point Damage без кода

Если не хочешь подписываться на делегат вручную — включи `bAutoApplyPointDamage = true` на самом компоненте: после броадкаста дополнительно автоматически вызовется `ApplyPointDamage`. Дефолт `false`.

### Интеграция с GAS (опционально)

Ядро и базовый Damage Routing полностью GAS-free — эта интеграция подключается только если она нужна, отдельным модулем `BallisticGAS`.

1. Создай `UBallisticGASDamageConfig` (Data Asset) — таблица маппинга `DamageProfileTag → FBallisticGASDamageEntry` (`DamageEffect` — какой `GameplayEffect` применить, `SetByCallerTag` — тег, в который подставится магнитуда урона из импакта)
2. В обработчике `OnBallisticDamageReceived` вызови `UBallisticGASDamageStatics::ApplyBallisticDamageViaGAS(Impact, Target, Config)`
3. `DamageProfileTag` берётся из профиля снаряда (`FBallisticProjectileProfile::DamageProfileTag`) — пустой тег = снаряд не привязан к GAS-урону

ASC (`AbilitySystemComponent`) цели резолвится только через стандартный `GetAbilitySystemComponentFromActor` — никакого ручного каста на конкретный класс персонажа.

---

## Ключевые механики

### Collision-LOD

Три уровня точности коллизий (L0/L1/L2) в зависимости от доли `DormantDistance` от игрока, плюс полная заморозка (dormant) за пределами `DormantDistance`. Дистанция измеряется от Pawn, не от камеры.

### Рикошет и пробитие

Направление и угол определяют исход (см. `CriticalRicochetAngleDegrees`). До трёх взаимодействий на снаряд (жёсткий предел). Пробитие толстых объектов защищено от повторного срабатывания через `PenetrationIgnoreClearDistance`.

### FX-подсистема

Импакты батчатся по тику, культятся по FOV и дистанции, приоритизируются по энергии и дистанции при превышении `MaxFXPerSecond`. Декали — пул с приоритетным вытеснением, дальность видимости управляется отдельно от движкового `FadeScreenSize`.

### Трейсеры — два независимых режима

- **Billboard** (`bTracersEnabled`) — дешёвые точки-спрайты, Niagara, CPU-симуляция. Подходят для сцен с массовым числом одновременных выстрелов.
- **Ribbon** (`bTrailRibbonEnabled`) — полноценная 3D-геометрия, собственный C++-рендер, видна под любым углом. Дороже, подходит для крупного плана/реалистичных шутеров. Приоритет отображения при переполнении пула учитывает дистанцию до камеры и принадлежность снаряда локальному игроку.

