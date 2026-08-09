# Architecture & Core Mechanics

## Обзор

Плагин состоит из трёх независимых подсистем:

- **Core** (`UBallisticSubsystem`) — симуляция полёта, коллизии, рикошет и пробитие. GAS-free, damage-agnostic — ядро не знает ни о системе урона, ни о косметике.
- **Cosmetic** (`UBallisticCosmeticSubsystem`) — визуальный и звуковой отклик: декали, частицы, звук, трейсеры.
- **Damage Routing** (`UBallisticDamageRoutingSubsystem`) — маршрутизация урона к целям, опционально интегрируется с GAS через отдельный модуль `BallisticGAS`, без обязательной зависимости.

Ядро построено на SoA-архитектуре (Structure of Arrays) с sparse/dense-индексацией и generation handle, параллельной обработкой через `ParallelFor`, и адаптивной системой LOD, снижающей стоимость коллизий и косметики по мере удаления от игрока.

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

Полный список настраиваемых параметров — в [`CONFIGURATION.md`](./CONFIGURATION.md).
