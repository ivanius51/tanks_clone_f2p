# Роадмап TanksClone

Finished sections are in `docs/ROADMAP_ARCHIVE.md` (not read at session start).

Статуси: ✅ зроблено · 🔧 у роботі · ⬜ заплановано

## ⬜ Мережева гра (Steamworks)

Залежить від ядра без рушія (MVVM) і детермінованої симуляції з фіксованим кроком: без них мережа неможлива.

- ⬜ **Порт мережі** (інтерфейс у моделі, без Steam і рушія): з'єднання, надсилання команд гравців, отримання стану, події «гравець приєднався / вийшов». Реалізації: локальна (тести, гра на одному ПК), LAN (UDP), Steam.
- ⬜ **Модель синхронізації:** спершу детермінований lockstep (усі виконують однакові команди з однаковим насінням, по мережі йдуть лише команди; перевірка хешу стану кожні N кроків); альтернатива для PvP — сервер-авторитет (хост рахує, клієнти отримують знімки стану й інтерполюють).
- ⬜ **Steamworks SDK:** лобі (`ISteamMatchmaking`), запрошення друзів і приєднання з оверлея, Steam Networking Sockets з ретранслятором (relay), тож не потрібні відкриті порти; SDK підключається лише в збірці для Steam (окрема конфігурація), інші збірки працюють без нього.
- ⬜ **LAN:** пошук гри в локальній мережі (broadcast), без Steam.
- ⬜ **Режими:** спершу кооператив (2 гравці проти хвиль, спільна база), потім PvP (команди, захоплення бази).
- ⬜ **Необов'язково:** досягнення Steam, хмарні збереження профілю (Steam Cloud), таблиці рекордів.
- ⬜ **Тести:** два екземпляри симуляції з локальним портом і боти; перевірка, що стани збігаються (хеш) після N хвилин гри; симуляція затримки й втрат пакетів.

## ⬜ Баланс і складність

- ⬜ **Ціни апгрейдів:** зараз лінійні (`base_cost + cost_step × рівень`). Порахувати дохід за рівень (монети за ворогів + бонус за перемогу) і підібрати ціни так, щоб за проходження кампанії можна було купити ~60-70 % апгрейдів; розглянути геометричне зростання цін.
- ⬜ **Крива складності ворогів:** таблиця «рівень → кількість ворогів, max_alive, інтервал, частка Light/Medium/Heavy/Boss»; плавне зростання, рівні-передишки, бос на кожному N-му рівні. Перевіряти ботом: час проходження й кількість втрат HP по рівнях.
- ⬜ **Економіка боєзапасу:** зі стартовими 20 снарядами і бонусами `A` гравець не повинен застрягати без снарядів; кулемет як запасний варіант. Підібрати шанс і кількість снарядів у бонусах і після вбивства ворога (дроп).
- ⬜ **Режими складності** (легкий / звичайний / важкий): множники здоров'я й шкоди ворогів, швидкості куль, кількості ворогів і монет у JSON (`configs/difficulty.json`), вибір у налаштуваннях і окремий рекорд для кожного режиму.
- ⬜ **Баланс зброї й стін:** DPS гармати й кулемета, скільки пострілів на прохід у цеглі й дереві, чи не робить кулемет гармату зайвою.
- ⬜ **Автоперевірка балансу:** бот проходить рівні на кожному режимі складності N разів із різним насінням; звіт: відсоток перемог, середній час, залишок HP і снарядів.

## ⬜ Якість і правила

- ✅ Правила в `.claude/rules/` (іменування, коментарі §19, архітектура, процес). Нове пишемо за ними; коментарі приводимо до правил лише у файлах, які й так редагуємо.
- ⬜ **Перейменувати константи** з `UPPER_SNAKE_CASE` на `PascalCase` (наприклад `PLAYER_NAME` → `PlayerName`, `TILE_SIZE` → `TileSize`).
- ⬜ Самоперевірка коду (code review) перед завершенням кожної задачі, за чеклістом із `workflow.md`.
- ⬜ Doc-коментар `/** */` для кожного головного класу.
- ⬜ **Data-driven:** everything tunable in JSON (input actions, `configs/ui`, balance, resource ids, tables, defaults, strings): audit and one work order in `docs/DATA_DRIVEN_PLAN.md`.

## ⬜ Ядро, незалежне від рушія (MVVM)

Мета: логіка підключається до будь-якого рендера (Cocos, веб, Android, інший рушій). Робити до скролінгу: розділення логічних сутностей і вузлів потрібне й для віртуалізації.

1. Виділити **Model**: чистий C++ без `cocos2d` (сутності, статистики, бафи, вміння, AI, спавнери, хвилі, правила рівня), свої типи вектора й прямокутника, симуляція з фіксованим кроком і детермінованою випадковістю.
2. **Порти** (інтерфейси): рендер, ввід, аудіо, сховище, джерело конфігів, годинник. Cocos реалізує їх у шарі View.
3. **ViewModel**: значення HUD, прогрес, події (знищено ворога, баф, хвиля). View лише відображає.
4. Заміна `cocos2d::Value` у конфігах на нейтральне представлення JSON, заміна планувальника рушія власним тактом.
5. Заміна синглтонів на ін'єкцію залежностей (контекст гри).
6. Тести й боти працюють на моделі без вікна, на максимальній швидкості, з насінням випадковості.

### Progress (2026-11-20)

- ✅ No file-level `USING_NS_CC`; namespaces by layer: `game::model`, `game::view::cocos` (scenes, HUD, widgets).
- ✅ Own math types `model::Vec2f`, `RectF`, `Color` + Cocos converters (`ui/CocosConvert.h`).
- ✅ Seeded random `model::Random` for all gameplay randomness; autotests use fixed seeds.
- ✅ Config interface `model::IConfigNode` (typed `GetValue<T>`, `GetValueOr`, `TryRead`; vectors, maps, enums by name, types with `FromConfig`). `core/JsonConfigNode` is the only JSON code.
- ✅ `GameContext` owns all 10 managers (typelist, ordered creation, reverse destruction, shutdown before the engine); `GetInstance()` singletons removed. ⬜ Model takes managers explicitly; ⬜ engine managers behind ports.
- 🔧 Loaders → plain structs: ✅ AI settings; ⬜ enemies and stats; ⬜ levels; ⬜ object factory / `Configure`.

### Simulation into the model (plan, 2026-11-24)

Layers like a classic game engine: the Model owns all state and rules; the engine supplies services through ports (clock, physics/collision, render, audio, storage, background I/O); the View only draws.

- ✅ Clock: `model::SimulationClock` advanced once per engine step by `CocosClockDriver`; `model::Cooldown` keeps timestamps; `model::Scheduler` runs callbacks at a game time.
- ✅ Events: `model::GameEvents` (per-level log) fed by game code: `enemy.spawned/destroyed/killed`, `pickup.taken`, `tank.hit`, `tank.hit.front|side|back`, `player.hit`, `bullet.hit_wall`, `bullet.hit_tank`, `bullet.bounced`, `wall.chipped`, `wall.destroyed` (`events::Report`). Level scripts wait on them in `until`; the bot, achievements and tests can read the same log.
- ⬜ Pure deciders: `model::HitResolver` (bullet + tank + facing -> side, damage); the Cocos code calls it instead of the inline logic, one case at a time, each with unit tests and the level tests as a safety net.
- ⬜ Collision is the project's own rectangle code (no Cocos physics): move it to `model::CollisionWorld` on model rectangles; nodes follow the positions.
- ⬜ Entity state into the model: pickups and spawner first, then bullets, tanks last; the Cocos object becomes a view of a model entity id.
- ⬜ One `Simulation::Step(dt)` with a fixed step and a fixed order (input, AI, movement, collision, events), driven by the clock driver; bots and tests run it without the engine.
- ⬜ Render port: the model emits "effect X at P", "sound Y", "camera shake"; a view adapter turns them into Cocos calls.
- Background threads only for I/O (config and texture loading, saves); the model stays single-threaded and deterministic.

### Own base library (`Classes/templates/`, plan 2026-11-24)

Rules: code-style.md, "Own templates". Order:
1. ✅ (2026-11-24) Folder and renames: `TypeList` → `TTypeList`, `ManagerSet` → `TManagerSet` moved to `templates/`; rule for the `T` prefix.
2. 🔧 Callbacks: `TFunction<Sig, InlineSize>` (default 128 bytes inline, build fails if a capture is bigger) and `TFunctionRef` done (2026-11-24) and used everywhere except the UI glue. Left on `std::function`: `ui/MenuBuilder`, `ui/SettingsPanel`, `ui/ActionListener`, `ui/Widgets`, `scenes/EditorScene`, `ResourceManager::ReadFileAsync`, `tests/MenuTests`: their lambdas capture other functions, which does not fit inline; fix by capturing pointers or shared state, then `tools/clang_check.py` flags any `std::function`. `TDelegate`/`TMulticastDelegate` wait for their first user.
3. 🔧 `TStoredString` done (2026-11-24; persistent pool, `Intern`/`Find`, 4-byte id); first user `GameEvents`. ⬜ Transient pool, `constexpr` names, `ResourceId`, ids of objects, enemies, buffs, upgrades. `TStoredStringView` only if a non-owning variant is needed beside `std::string_view`.
4. ⬜ Containers: `TArray`/`TMap` as `std::pmr` aliases; `TMap` is a sorted flat map (stable iteration for replays); arenas Frame/Level/Session owned by the simulation.
5. ⬜ More templates, each when its first user appears:
   - `TSlotMap<T>` + `TEntityHandle` (index + generation): model entity ids for "the Cocos object is a view of a model entity", safe after delete.
   - `TObjectPool<T>`: node pools for row virtualization and bullets.
   - `TEnumArray<E, T>`: array indexed by an enum with the `static_assert` on its range (replaces enum value lists).
   - `TEnumFlags<E>`: type-safe bit flags (`ObjectEnums`, `AiSwitches`).
   - `TRingBuffer<T, N>`: event log, input history, replay buffer, without allocation.
   - `TSmallArray<T, N>`: inline storage that spills to the arena (hit lists, neighbor lists).
   - `TScopeGuard` and `TObserved<T>` (value with change event, for the ViewModel).
   - `RefCounted`/`Handle` and `ConfigCatalog`/`WeightedTable` move here with the `T` prefix when touched anyway.

### After release: binary configs

- JSON stays the source format: it is what we edit and what Debug/editor builds read. Shipping builds read binarized configs packed into one or a few archives.
- Binary configs are read straight into memory objects: faster loading, smaller files (strings such as enum names and ids become ints, via the string store below).
- Localization stays text (not binarized). Today `StringsManager` loads `configs/strings/<language>.json` (flat `id → text`) through `ConfigReader` as a `cocos2d::ValueMap` per language and looks strings up by `std::string` id; it moves to `IConfigNode` and string-store ids in the loaders work.
- Every config type gets symmetric `Encode`/`Decode` (write/read) in the adapter layer, next to the JSON reader, so text and binary share one data model; a tool converts JSON → binary at build time, and tests check that JSON → binary → object gives the same object.
- File access goes through a small stream-based IO port (`std::istream`/`std::ostream`), not through filesystem calls spread across the code. Loaders get a stream, so the same loader works for files, packed archives and in-memory test data.

### Global string store (before the enemies/stats loaders)

- `TStoredString` (see "Own base library"): a small integer id for an interned string; compare and hash are int operations, `View()` gives a `std::string_view` into the pool.
- Two pools: persistent (config keys, ids, enum names; lives for the whole run) and transient (per level / per screen; cleared explicitly, ids from it are invalid after the clear).
- Everything else takes `std::string_view` instead of owning `std::string`; configs, loaders and the binary format store ids.

## ⬜ Open from infrastructure (the finished parts are in docs/ROADMAP_ARCHIVE.md)

- ⬜ Resolution: sprites 32x32 scale x2 (pixel art); sets for 1x/2x/4x, check `SHOW_ALL` on other aspect ratios, big fonts and HUD at 4K.
- ⬜ Real art instead of generated placeholders (base sprites: whole / ruins; enemy previews for the editor). Sounds are synthesized placeholders too.
- ⬜ Find unused files and frames; placeholder for missing configs.
- ⬜ Resource manager: binary configs through `WorkQueue`; streaming map chunks (nothing streams yet).
- ⬜ Still on the engine: `GameScene` / editor / test-runner keyboard listeners, `CommandType` (`ObjectEnums.h` includes cocos2d), config files read through `FileUtils` (`ConfigReader`).

## ⬜ Редактор рівнів (повернемося після HUD, бази й скролінгу)

Сьогодні редактор вміє: палітра з 8 тайлів, малювання мишею, лічильник ворогів, збереження, тест. Пише лише простий формат рівня (одна хвиля, типовий спавнер).

План:

1. **Спавнери.** Палітра отримує спавнери `E`, `1`-`9`. Клік по спавнеру відкриває панель:
   - пул: класи ворогів (Light / Medium / Heavy / Boss) та/або конкретні вороги зі списку `EnemyCatalog`;
   - обмеження: які вороги й класи **не** спавнити;
   - кольорове позначення спавнера на карті та підпис його номера.
2. **Хвилі.** Окрема панель: список хвиль (додати / видалити / перемістити), для кожної `delay`, `max_alive`, `interval` і рядки «спавнер × кількість». Підрахунок усього ворогів. Запис у розширений формат (`spawners`, `waves`).
3. **Конструктор ворога.** Створення нового файлу `configs/enemies/<Id>.json`: клас, статистики (повзунки), вміння (список із параметрами), бафи, AI, колір, нагорода, прев'ю танка. Збереження в writable-папку, `EnemyCatalog` підхоплює його без перезбирання.
4. **Конструктор бафів і вмінь** (за аналогією): редагування `buffs.json`, модифікатори статів, іконка.
5. **Налаштування рівня:** назва, фон / декор, база (див. нижче), розмір рівня (див. скролінг).
6. **Зручності:** undo/redo, заливка й прямокутник, дзеркалення карти, кілька власних рівнів (список слотів), імпорт / експорт файлу, перевірка досяжності (чи не замурований гравець / база).
7. **Тест-запуск** з будь-якої точки, повернення в редактор із тим самим станом.

## ⬜ Захист бази

База не обов'язкова: вмикається в конкретному рівні (рівень без бази працює як зараз).

- Новий тайл `B` (база) у карті, об'єкт `Base : PhysicsObject` (дані в `configs/base.json`: здоров'я, спрайт, спрайт руїн).
- Поле рівня `"base": { "health": 10 }`; без нього тайл `B` ігнорується або дає помилку валідації.
- Умови: знищено базу = поразка (як смерть гравця); перемога = всі хвилі знищено, а база жива.
- Вороги: AI-клас `BaseHunter`, що веде танк до бази (пошук шляху по сітці, обхід стін, розстріл цегли на шляху); частина ворогів може обирати між базою й гравцем.
- HUD: шкала здоров'я бази на верхній смузі, попередження при атаці.
- Бонуси: «ремонт бази», «укріплення» (стіни навколо бази на час), вміння «щит бази».
- Редактор: тайл бази, поле здоров'я в налаштуваннях рівня; валідація (одна база, досяжна для ворогів).

## ⬜ Скролінг і довгі рівні

Рівень це не завжди одне поле: у JSON задається розмір карти (будь-який), точка старту гравця, зони й спавнери по всій карті; гравець їде в усі боки.
- **Мінікарта** окремим елементом інтерфейсу: контур рівня, позиція гравця, бази, спавнерів і ворогів у радіусі огляду; відкривається повною картою.
- Формат рівня: size (стовпці × рядки), player_start, чанки або суцільні 
ows; сумісність із поточними одноекранними рівнями.

Мета: рівні довші за екран; віртуально існують усі танки й об'єкти, а на екрані малюється лише видима частина.

**Модель даних**
- Світ більший за екран: `rows` довільної висоти (наприклад 40-200 рядків), ширина лишається 16.
- Логічні сутності (`GameObject`) живуть незалежно від вузлів; вузол (`Node`) створюється лише для сутності, що потрапила у вікно.
- Статичні тайли (стіни, бонуси) зберігаються як дані карти (сітка), а не як мільйон об'єктів.

**Вікно відображення (віртуалізація)**
- Камера слідує за гравцем із плавним згладжуванням і межами рівня.
- Вікно = видимі рядки + **2 рядки зверху й знизу** запасу. Коли камера рухається, наступний рядок уже підготовлений, тож поява плавна, без стрибків.
- Рядок, що виходить із вікна, повертає свої вузли в **пул** і повторно використовується для рядка, що входить («перестановка»), а не створюється заново.
- Зберігається стан: пошкоджена стіна лишається пошкодженою, коли рядок повертається у вікно (стан у даних, вузол лише відображення).

**Симуляція**
- Вороги далеко від камери працюють у спрощеному режимі (рух і AI на сітці без вузлів, без анімацій); поруч із вікном (≤ N рядків) повноцінні.
- Колізії й пошук (`findObjectIf`) переходять на просторову сітку (бакети по рядках), щоб не перебирати все.
- Кулі: лише в активній зоні; за її межами видаляються.
- Спавнери працюють по всьому світу; хвилі можуть прив'язуватися до зон (тригер: гравець дійшов до рядка N).

**Інтеграція**
- HUD та миніатюра рівня (мінікарта: позиція гравця, бази, ворогів).
- Редактор: прокрутка карти, зміна висоти рівня, зони хвиль.
- Збереження прогресу всередині довгого рівня (чекпойнти).
- Продуктивність: тест на 200 рядках і сотнях ворогів, лічильник вузлів у режимі налагодження.

> Порядок: інфраструктура → ядро без рушія (MVVM) → захист бази → скролінг → редактор.

## ⬜ Далі в черзі

- Баланс цін апгрейдів і складності рівнів
- Бонуси-бафи на картах (тайли для щита, скорострільності)
- Музика й більше звуків; налаштування гучності
- Керування: геймпад, перебиндинг клавіш
- Кілька гравців (локальна кооперація)
- Мобільні платформи (екранні кнопки, масштабування HUD)
- Тести: автоматична перевірка конфігів (валідатор JSON при старті)
