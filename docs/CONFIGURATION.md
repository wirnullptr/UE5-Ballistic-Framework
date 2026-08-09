# Configuration Reference

Все настройки — в `Project Settings → Plugins → Ballistic Framework`, три раздела.

## Core

| Поле                             | Дефолт   | Описание                                                                                                                              |
| -------------------------------- | -------- | ------------------------------------------------------------------------------------------------------------------------------------ |
| `ProjectileDataAsset`            | —        | Data Asset с профилями снарядов. Обязателен — без него спавн падает.                                                                  |
| `SurfaceInteractionAsset`        | —        | Data Asset взаимодействия с поверхностями. Не назначен → рикошет/пробитие выключены.                                                  |
| `ParallelThreshold`              | 64       | Порог числа снарядов, с которого включается параллельная обработка.                                                                   |
| `BatchSize`                      | 256      | Размер батча для `ParallelFor`.                                                                                                       |
| `DormantDistance`                | 15000 см | Дистанция ухода снаряда в заморозку (dormant).                                                                                        |
| `WakeSweepSagTolerance`          | 5 см     | Точность retroactive-проверки при пробуждении из dormant.                                                                             |
| `MaxWakeSegments`                | 16       | Максимум сегментов wake-sweep.                                                                                                        |
| `CollisionLOD1Fraction`          | 0.33     | Доля `DormantDistance`, ближе которой — полная точность коллизий (L0).                                                                |
| `CollisionLOD2Fraction`          | 0.66     | Доля `DormantDistance` для перехода L1 → L2 (редкие line-трейсы).                                                                     |
| `CollisionLOD2ThrottleInterval`  | 3        | На L2 — трейс раз в N substep'ов.                                                                                                     |
| `PenetrationIgnoreClearDistance` | 100 см   | После пробития компонент игнорируется, пока снаряд не отойдёт на эту дистанцию (защита от повторного хита при пробитии толстых стен). |
| `RicochetRandomSeed`             | 0x5EED   | Seed детерминированного потока рикошета.                                                                                              |
| `MaxUnits`                       | 100000   | Максимум одновременно живых снарядов — определяет размер SoA-аллокации при старте.                                                    |

## Cosmetic

| Поле                                         | Дефолт  | Описание                                                                                                                                                                            |
| --------------------------------------------- | ------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `FXCullDistance`                             | 5000 см | Дистанция отсечения FX (FOV-aware).                                                                                                                                                 |
| `MaxFXPerSecond`                             | 64      | Token-bucket лимит FX в реальном времени (независим от масштаба игрового времени — работает и в bullet-time).                                                                       |
| `MaxDecalsInWorld`                           | 500     | Кап пула декалей.                                                                                                                                                                   |
| `DecalMaxVisibilityDistance`                 | 5000 см | Дальность видимости декали от камеры (движковый механизм `FadeScreenSize` не даёт настроить дальность в см напрямую — плагин реализует собственный per-frame контроль поверх него). |
| `SoundNearThreshold`                         | 5000 см | Ближе этой дистанции — звук без задержки распространения.                                                                                                                           |
| `SpeedOfSoundCmPerSec`                       | 34300   | Скорость звука для расчёта задержки на дальних попаданиях.                                                                                                                          |
| `FXLibrary`                                  | —       | Data Asset с профилями импакт-эффектов по типу поверхности.                                                                                                                         |
| `TracersEnabled` (`bTracersEnabled`)         | false   | Билборд-трейсеры (лёгкие точки, GPU-Niagara мост, CPU-симуляция).                                                                                                                   |
| `TracerNiagaraSystem`                        | —       | Niagara-система для билборд-трейсеров. Не назначена → трейсеры выключены.                                                                                                           |
| `MaxTracerBufferSize`                        | 100000  | Потолок буфера позиций трейсеров.                                                                                                                                                   |
| `TrailRibbonEnabled` (`bTrailRibbonEnabled`) | false   | Ribbon-трейлы — полноценная 3D-геометрия хвоста, видна с любого ракурса (в т.ч. полёт на камеру, где билборд-спрайты не видны). Независим от билборд-трейсеров, можно включать оба. |
| `MaxTrailSlots`                              | 512     | Лимит одновременных активных лент (независим от `MaxTracerBufferSize`).                                                                                                             |
| `MaxTrailPoints`                             | 16      | Точек истории на одну ленту.                                                                                                                                                        |
| `TrailCullDistance`                          | 5000 см | Дистанция отсечения лент от камеры.                                                                                                                                                 |
| `TrailMinPointDistance`                      | 5 см    | Минимальное расстояние между соседними точками истории — избегает скопления точек у медленных/остановившихся снарядов.                                                              |
| `TrailDefaultMaterial`                       | —       | Материал ленты по умолчанию (должен использовать Vertex Color для fade/цвета).                                                                                                      |
| `TrailDefaultWidth`                          | 20      | Базовая ширина ленты.                                                                                                                                                               |

## Damage Routing

| Поле                     | Дефолт | Описание                                                                                                                              |
| ------------------------ | ------ | ---------------------------------------------------------------------------------------------------------------------------------------- |
| `bFallbackToPointDamage` | true   | Если задетый актор не зарегистрирован как цель — применяется стандартный `ApplyPointDamage` ("урон из коробки" без ручной настройки). |

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
