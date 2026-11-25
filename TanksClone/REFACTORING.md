# Рефакторинг TanksClone

## Структура коду (`Classes/`, простір імен `tanks`)

```
AppDelegate (глобальний, вхід cocos) ──► MainScene ──► ObjectsFactory.createFromConfig("configs/player_green_tank.json")
                                                          │ володіє об'єктами (unique_ptr)
KeyboardController ─► CommandsManager ─(за ім'ям / за типом)─► GameObject ◄─ PhysicsObject ◄─ MovableObject ◄─ Tank
ConfigReader (JSON, кросплатформно) ─► StringsManager
```

| Файл | Призначення |
|------|-------------|
| `ObjectEnums.*` | `ObjectType`, `NodeType`, `CommandType`, `ActionTag`, `VehicleType`; розбір значень з JSON за назвою (`"Tank"`) або числом |
| `GameConstants.h` | `constexpr` шляхи й імена замість «магічних» рядків |
| `ObjectsFactory.*` | створення об'єктів з опису/конфігу, власність, пошук; параметри вузла (`position_normalized`, `move_animation`, ...) |
| `GameObject` → `PhysicsObject` → `MovableObject` → `Tank` | дані об'єкта, AABB-колізія, рух/поворот, танк |
| `CommandsManager`, `GameCommand` | черга команд, доставка щокадру |
| `KeyboardController` | клавіші → команди для гравця |
| `ConfigReader`, `StringsManager`, `DataConverter` | JSON-конфіги, локалізація, перетворення `Value` |

## Що зроблено

**Помилки (етап 1):** UB при видаленні через базу без віртуального деструктора; `update` у scheduler після знищення об'єкта; розіменування `nullptr` у `CommandsManager`; неоднозначні перевантаження `GameCommand()`/`addCommand()`; `CMakeLists.txt` з неіснуючим `HelloWorldScene.cpp`.

**Архітектура (етап 2):**
- Весь код у `namespace game` (раніше в `cocos2d` через `NS_CC_BEGIN`).
- Макроси `FACTORY`/`COMMAND`/`KEYBOARD`/`CONFIGS`/`STRINGS`, `UP_DEGREES` тощо, `#define` кутів і аудіо-прапорці замінено на `constexpr`/виклики `X::getInstance()`; рядкові шляхи винесено в `GameConstants.h`.
- Назви: `IGameObject`/`IPhysicsObject`/`IMovableObject` → `GameObject`/`PhysicsObject`/`MovableObject` (це не інтерфейси); `eObjectType::OBJECT_TYPE_VEHICLE` → `ObjectType::Vehicle` і т.д.; видалено порожні enum (`eWallType`...).
- Власність: фабрика тримає `unique_ptr`, назовні віддає невласні вказівники; `Tank::create` повертає `unique_ptr`. Зайві `getObjects()`/`getObjectsByType()` прибрано, замість них `findObject` і `forEachObject`.
- Фабрика керується даними: `player_green_tank.json` тепер справді використовується (`createFromConfig`), тестовий `ValueMap` з `MainScene` видалено. Реалізовано `position_normalized` і `move_animation` (анімація з конфігу). У JSON рядкові ID (`"Vehicle"`, `"Sprite"`, `"Tank"`), виправлено `frames_coun` → `frames_count` (7 кадрів) і `scale` (раніше `1.0` не застосовувався, бо перевірявся тип FLOAT, а JSON дає DOUBLE).
- 8 класів-сеттерів параметрів замінено таблицею функцій у `ObjectsFactory.cpp`.
- `ConfigReader` без WinAPI: `FileUtils::listFilesRecursively`, шляхи через `/`. Працює на всіх платформах.
- Ввід: команди доставляються всі щокадру (було по одній на 20 мс, черга могла відставати); команда без імені, але з типом, іде всім об'єктам цього типу (раніше мовчки губилась); відпустили останню клавішу при затиснутій іншій, танк їде в напрямку тієї, що лишилась (раніше зупинявся). Деструктор `KeyboardController` більше не звертається до `Director` під час завершення програми.
- Видалено мертвий код: `LoadingScene`, `MenuScene` (копії шаблону HelloWorld, ніде не підключені), порожній `ObjectTypes.cpp`, невикористане `registerPlayerKeyAction`, `ValueToStringMap`, аудіо-блоки в `AppDelegate`.
- `.vcxproj` і `.vcxproj.filters` оновлено під нові/видалені файли.

## Що лишилось (не рефакторинг, а нові можливості / рішення)

- `Shoot` ігнорується; немає куль, стін, землі, бонусів, здоров'я; `PhysicsObject::isCollidedTo` ніхто не викликає.
- Рух: `Action`-и cocos + прапорці `mIsRunning`/`mIsRotating`, напрямок рахується до завершення повороту. Варто замінити явним автоматом станів, коли з'являться колізії.
- Синглтони лишилися (без макросів). Наступний крок: один `GameContext`, який створює `AppDelegate` і передає в сцени.
- Рушій `cocos2d/` лежить прямо в репозиторії: краще git submodule.
- Візуал: піксельна графіка потребує `setAliasTexParameters` і цілочисельного масштабування; зараз стоять шаблонні 1024×768 з `NO_BORDER`.

---

## Етап 3: гра, меню, редактор (поточний стан)

**Екрани:** `LoadingScene` → `MenuScene` → `GameScene` / `UpgradesScene` / `SettingsScene` / `EditorScene`. Кнопки, панелі й діалоги: `Widgets.*` (миша та клавіатура: стрілки/W S, Enter).

**Геймплей:** `Bullet`, `Wall` (кілька стадій пошкодження), `Bonus` (патрони, ремонт), `Tank` (патрони, броня, перезарядка), вибухи в `Effects`, звуки через `AudioManager`.

**Вороги:** `TankAi` (`Wander` блукає, `Hunter` іде до гравця; обидва стріляють, коли гравець на лінії вогню). Типи ворогів задаються JSON: `enemy_basic`, `enemy_fast`, `enemy_heavy`. `EnemySpawner` випускає хвилі через спавн-пойнти `E` (усього / одночасно / інтервал).

**Карти:** `Resources/configs/levels/level_N.json`: 16×11 символів (`# S W` стіни, `P` гравець, `E` спавн ворогів, `A R` бонуси) і блок `enemies`. Редактор зберігає власну карту в writable-папку (`custom_level.json`), у ній не нараховуються монети й прогрес.

**Збереження:** `ProfileManager` (`profile.plist` у writable-папці): монети, рекорд, пройдені рівні, апгрейди, мова, звук. **Апгрейди:** `configs/upgrades.json` + `UpgradeManager` (ємність патронів, шкода, броня, перезарядка).

**Перевірено:** повна збірка (Debug|Win32, v143), запуск, меню, гра: рівень, HUD, вороги, бонуси, стіни. **Не перевірено вручну:** редактор, магазин апгрейдів, екрани перемоги/поразки, пауза, збереження між запусками, ефективність AI.

**Далі:** баланс цін і складності, вибір кількох власних карт, вибір типів ворогів у редакторі, музика, більше типів тайлів і землі, прибрати невикористаний параметр `aZOrder` у `GameScene::spawnObject`.
---

## Етап 4: розширювані вороги, скіли, бафи, спавнери

**Сутності (кожна окремий клас, дані в JSON):**
- `Stats` (`StatSet`, `StatModifier`): MaxHealth, Damage, Armor, Speed, ReloadTime, MaxAmmo. Фінальне значення = (база + сума `add`) × добуток `mul`.
- `Buff` (`BuffCatalog`, `BuffSet`): `configs/buffs.json`. Баф має тривалість (або постійний), модифікатори статів і/або невразливість. Бафи накладають вміння, бонуси й апгрейди.
- `Ability`: вміння танка. Типи: `Shoot`, `Buff` (дає баф з перезарядкою). Перелік вмінь у `"abilities"` конфігу танка; перше основне (пробіл), решта по `E` (гравець) або випадково (AI).
- Апгрейди тепер теж дані: `configs/upgrades.json` (`modifiers` одного рівня), у грі це постійні бафи `upgrade_<id>`.

**Вороги:** `EnemyType` це **клас** (Light / Medium / Heavy / Boss), а не конкретний ворог. Конкретний ворог = файл `configs/enemies/<Id>.json`; `EnemyCatalog` знаходить їх автоматично.

Щоб додати нового ворога, створіть `configs/enemies/Tank5.json`: статистики (`health`, `damage`, `armor`, `speed`, `reload_time`, `max_ammo`), `abilities`, `buffs`, `ai`, `enemy_type`, `score`, `coins`, `color`, `name`. Код чіпати не треба.

**Нове вміння:** клас від `Ability` + один рядок у таблиці `Ability.cpp`. **Новий баф:** запис у `buffs.json`. **Нова статистика:** значення в `StatType` + назва в `Stats.cpp`. **Новий клас ворогів:** значення в `EnemyType` + назва в `ObjectEnums.cpp`.

**Спавнери й хвилі (формат рівня):**
- На карті спавнери: `E` (типовий) і цифри `1`-`9`.
- `"spawners": { "1": { "enemies": ["Boss"] }, "E": { "types": ["Light","Medium"], "exclude": ["Basic"] } }`: пул спавнера це класи та/або конкретні вороги, `exclude` і `exclude_types` обмеження.
- `"waves": [ { "delay", "max_alive", "interval", "spawns": [ { "spawner": "E", "count": 5 } ] } ]`: наступна хвиля стартує, коли знищено попередню.
- Без `waves` діє проста форма (`"enemies": {total, max_alive, ...}`), яку пише редактор. Приклад складної форми: `level_6.json` (окремий спавнер боса, три хвилі).

**Далі:** винести це в редактор (спавнери `1`-`9`, пули, обмеження, хвилі, конструктор ворога); показ бафів і кулдауну вмінь у HUD.