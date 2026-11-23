# Роадмап TanksClone

Статуси: ✅ зроблено · 🔧 у роботі · ⬜ заплановано

## Зроблено

- ✅ Рефакторинг: простір імен `tanks`, `constexpr` замість макросів, `unique_ptr`, кросплатформний `ConfigReader`
- ✅ Меню, налаштування (мова, звук), магазин апгрейдів, пауза, екрани результату, збереження профілю
- ✅ Геймплей: кулі, стіни (кілька стадій руйнування), бонуси, колізії з вирівнюванням по сітці
- ✅ Статистики, бафи, вміння як окремі сутності; вороги як JSON (`configs/enemies/`), `EnemyType` = клас ворога
- ✅ Спавнери (`E`, `1`-`9`) з пулами та обмеженнями, хвилі, окремий спавнер боса (рівень 6)
- ✅ Рівні 1-6, редактор у простому форматі (одна хвиля)

## Поточний стан (для передачі між сесіями)

- Збірка: `MSBuild proj.win32\TanksClone.sln /p:Configuration=Debug /p:Platform=Win32 /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0.22621.0 /m`. Перед збіркою зупинити `TanksClone.exe`.
- Тести (`TanksClone.exe --test` у Debug і Test, звіт `test_report.txt`, усе проходить): 43 юніт-тести, 12 автотестів рівнів (бот `PlayerBot`, сценарій без бота, натовп ворогів), 32 кроки автотестів меню (разом із вимірюванням FPS). На рівнях 1-6 бот програє (очікувано, перевіряється, що вороги з'являються); тестові поля він виграє. Тести пишуть профіль і свій рівень у тимчасову папку (`%TEMP%\TanksCloneTests`), справжній профіль гравця не чіпають.
- CCD куль і ліміт FPS перевірено, `--test` проходить.
- Структура файлів, збірка, dist і реліз описані в `README.md`.
- Робочий лише `proj.win32`. `proj.android`, `proj.ios_mac`, `proj.linux` лишились шаблонними (посилаються на `HelloWorldScene.cpp`, `MyGame`, `org.cocos2dx.*`) і не збираються.
- Далі: справжній арт і шрифти (див. «MVP-прибирання»), потім за порядком нижче.

## ✅ MVP-прибирання (листопад 2026)

- ✅ Прибрано залишки шаблону Cocos: `HelloWorld.png`, `CloseNormal/Selected.png`, атлас `minimalist` (запозичені кадри), порожня `Resources/res/`, рядки `TEXT_1..3`, ліцензійні шапки й коментарі шаблону в `AppDelegate`/`main`, назви в `game.rc`.
- ✅ Іконка застосунку (танк) замість іконки Cocos: Windows `.ico`, Android, iOS, macOS (`tools/gen_app_icons.py`).
- ✅ Статистика рушія лише в Debug (F3), `--test` лише в Debug (`core/BuildInfo.h`).
- ✅ Ресурси знаходяться поруч з exe (`<exe>\Resources\`), а не в робочій папці.
- ✅ Структура `Resources/`: `configs/`, `textures/` (`common`, `player`, `abilities`, `enemies/<Id>`, `tilesets/<name>`), `sounds/`, `fonts/`. Вихідні кадри в `art/`.
- ✅ Атласи: `tools/pack_atlases.py` збирає `.plist` + `.png` з `art/` (без сторонніх пакетів, `--check` для перевірки). Імена кадрів унікальні; кадри тайлсетів з префіксом (`desert/wall_brick`).
- ✅ Маніфест ресурсів `configs/resources.json` + `ResourceManifest`: спільні атласи вантажаться на старті; рівень вантажить лише свій тайлсет і атласи ворогів, яких можуть вислати його хвилі (`AtlasScope`, підрахунок посилань), і вивантажує їх при виході. Ворог указує свій атлас полем `atlas`, рівень тайлсет полем `tileset`.
- ✅ Тайлсети `field` (рівні 1-3) і `desert` (4-6): земля з 4 варіантів тайлів замість малювання кодом.
- ✅ Унікальна тимчасова графіка (`tools/gen_placeholder_art.py`): у кожного ворога свій спрайт (без тонування), гравець, кулі, вибухи, бонуси, маркер спавну, іконки вмінь і бафів, стіни з 3 стадіями руйнування.
- ✅ Звуки синтезовані скриптом (`tools/gen_placeholder_sounds.py`); задіяно нові: старт рівня, руйнування стіни, вибух боса. Попередні (ймовірно запозичені) звуки перенесено в `../reference_assets/sounds/` поза проєктом.
- ✅ Збірка: post-build `tools/make_dist.ps1` складає `dist/Debug`, `dist/Test` і `dist/Release` лише з потрібним (exe, DLL рушія, `Resources/`; Debug + `.pdb`, Release + VC runtime). Архів: `tools/make_release.ps1`. `.gitignore` для збірок.
- ✅ Тести ресурсів: кожен кадр із конфігів є в атласі, рівень вантажить лише потрібне, атлас тримається, поки потрібен хоч одній сцені, два тайлсети не заважають один одному.

**Release-збірка:**
- ✅ У рушії вимкнено 3D-фізику (bullet) і Box2D: їхніх win32-бібліотек у `cocos2d/external` немає, а гра їх не використовує. Зміни в рушії: `cocos2d/cocos/2d/cocos2d_headers.props` (`CC_USE_3D_PHYSICS=0`) і `cocos2dx.props` (без `libbox2d.lib`, `libbullet.lib`). Оригінали: `../reference_assets/*.props.orig`. 2D-фізика (chipmunk) лишилась, бо її бібліотека є.
- ✅ Третя конфігурація `Test`: оптимізації й рушій Release, але з `--test` і F3 (`TANKS_DEBUG_TOOLS`, `HasDebugTools` у `core/BuildInfo.h`); `dist/Test` з `.pdb`. Рушій для неї збирається як Release (мапінг у `.sln`). `--test` у Test ~29 с проти ~37 с у Debug. CMake цю конфігурацію не має (CMake-збірка не підтримується).
- ✅ Release збирається, `dist/Release` (~7 МБ у zip, з VC runtime) запускається з будь-якої робочої папки; `--test` у Release ігнорується. Архів: `tools/make_release.ps1` → `dist/TanksClone-<версія>-win32.zip`.

**Потребує справжнього арту / ліцензій (перед публічним релізом):**
- ⬜ Уся графіка — згенеровані заглушки 32×32: спрайти 6 танків (3 кадри руху), кулі, 5 кадрів вибуху, 4 бонуси, 5 іконок вмінь, 2 тайлсети (земля, цегла, камінь, дерево), маркер спавну, іконка застосунку.
- ⬜ Звуки — прості синтезовані заглушки (12 шт.).
- ✅ **Шрифти:** замість `Marker Felt.ttf` (Apple) і `arial.ttf` (Microsoft) з шаблону — OFL-шрифти з українською кирилицею: Press Start 2P (заголовки, піксельний) і PT Sans (інтерфейс), ліцензії `fonts/OFL-*.txt`. Старі перенесено в `../reference_assets/fonts/` (поза git).
- ✅ Українська локалізація (`configs/strings/ukrainian.json`), мова в налаштуваннях: English → Українська → Русский. Тест: у кожній мові є всі рядки англійської.
- ⬜ Нема спрайтів бази й прев'ю ворогів для редактора (з'являться разом із цими функціями).

## ✅ Кулемет і руйнування стін по клітинках (листопад 2026)

- ✅ **Друга зброя гравця — кулемет:** список `weapons` у конфігу танка (`id`, `name`, `icon`, `bullet`, `reload_time`, `damage`, `uses_ammo`), перемикання Q / Tab, кожна зброя перезаряджається окремо. Кулемет: без снарядів, перезарядка 0.12 с, шкода 1, малі кулі (`bullet_mg.json`), камінь не бере, цеглу кришить повільно. Вороги без `weapons` мають одну гармату, як раніше.
- ✅ HUD: слот 0 показує іконку активної зброї й клавішу Q; рядок боєзапасу показує назву кулемета й INF, коли він активний.
- ✅ Бот перемикається на кулемет без снарядів і назад, коли снаряди з'являються.
- ✅ **Стіни з підклітинок:** чиста модель `model/WallMask` (без рушія): сітка клітинок зі здоров'ям, удар вибиває клітинки з боку, звідки прилетіла куля (ширина з `chip_width` кулі, глибина `chip_depth`, шкода `chip_damage`). Вибух зачіпає й сусідню стіну, якщо куля влучила між ними. Колізії танків (`blocks`) і CCD куль (`sweepHit`) ідуть по клітинках, що лишились, тож через вибитий прохід можна проїхати.
  - Цегла: 4×4 клітинки по 2 HP, снаряд знімає всю ширину на 2 ряди (2 снаряди = прохід, як і раніше). Дерево: 2×2 по 1 HP (1 снаряд). Камінь: лише кулі з `breaks_stone` (поки таких немає).
  - Рендер: клітинки — дочірні спрайти з того самого кадру атласу; пошкоджені затемнені. Старі кадри `*_damaged_1..3` прибрано.
- ✅ Тести: юніт (вибивання з боку, глибина й здоров'я клітинок, прохідність, CCD по клітинках, вибух між двома стінами, камінь; перемикання зброї, кулемет без снарядів, вороги з однією гарматою); автотести рівнів (бот виграє лише кулеметом; бот не застрягає перед суцільною цегляною стіною; танк, їдучи й стріляючи, пробиває прохід і проїжджає); автотести меню (налаштування: мова, звук, FPS; магазин: без монет і покупка; рівень: пауза, продовжити, екран результату, вихід у меню з паузи; редактор: збереження; свій рівень).
- ⬜ Анімація уламків при вибиванні, звук кулемета, окрема куля «ламає камінь» для боса.

## ✅ Виправлено: танки один в одному, ривок при повороті, ліміт FPS (листопад 2026)

- ✅ **Танки застрягали один в одному.** Причини: спавнер перевіряв точку лише на початку спалаху (0.5 с), і друга хвиля могла з'явитись у тій самій точці; а танк, що вже перекривав іншого, не міг зробити жодного кроку. Тепер: точка резервується на час спалаху й перевіряється ще раз перед появою (зайнята — ворог стає в чергу знову); крок дозволено, якщо перекриття не росте (можна роз'їхатись, не можна заїхати глибше); танки, що вже перекриваються, самі розходяться по меншій осі, не швидше за свою швидкість і не в стіну.
- ✅ **Ривок при повороті.** Вирівнювання по сітці тепер не миттєве: танк плавно з'їжджає до центру клітинки зі швидкістю руху і зупиняється, якщо там зайнято.
- ✅ **Ліміт FPS не працював у грі.** Ліміт діяв глобально (і в меню), а вимкнення vsync не спрацьовувало: гра викликала власну копію GLFW, не ту, що в `libcocos2d.dll`. Тепер меню завжди 60, рівень вмикає ліміт з профілю при вході й повертає 60 при виході; vsync на Windows керується через `wglSwapIntervalEXT`; таймер Windows 1 мс (`timeBeginPeriod`), щоб `Sleep` у головному циклі не округлював кадр до 15.6 мс.
  - На ноутбуці з гібридною графікою (рендер NVIDIA, дисплей AMD 144 Гц) рівень без ліміту дає ~72 FPS при вимкненому vsync: це обмеження шляху виводу, не гри.
- ✅ Тести: юніт (танки в одному місці роз'їжджаються, не в стіну; крок углиб заборонено; поворот без стрибка, бокове зміщення не швидше за рух; спавнер не ставить ворога у ворога; інтервал кадру для рівня й меню); автотести (натовп ворогів 40 с: жодна пара не перекривається довше 1 с; FPS рівня з лімітом 30 ≈ 30, з 120 і без ліміту > 66 і vsync вимкнено, меню ≈ 60).

## ✅ Рух без вирівнювання по сітці (листопад 2026)

- ✅ Поворот лише змінює напрям, позиція не змінюється (ні миттєво, ні плавно).
- ✅ Ковзання по кутах: крок уперед, заблокований краєм перешкоди, зсуває танк убік до проходу, якщо вистачає зсуву до `corner_assist` пікселів (JSON, за замовчуванням третина клітинки); інакше танк зупиняється.
- ✅ AI ворогів відкладає поворот упоперек руху до центру клітинки (або поки не впреться, але не довше 1 с).
- ✅ Бот без ворогів від'їжджає до свого ряду, а не стоїть на точці спавну (зайнята точка нікого не випускає).
- ✅ Тести: поворот без жодного зсуву; танк, що зачепив кут проходу, заїжджає в нього; танк, що здебільшого впирається в стіну, зупиняється.
- ✅ Історія версій `docs/history/` (знімки, кліп, `CHANGELOG.md`), режим `--showcase <папка>` (Debug і Test): знімки меню, налаштувань, магазину, редактора, рівнів 1-6, боса і 5-секундний кліп бота; `tools/make_gif.py` збирає GIF без сторонніх програм.

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

## ✅ Виправлено за відгуками

- Подвійний вибух при знищенні блока: тепер вибух лише блока; вибух кулі тільки для великих куль (explosion_on_hit) у точці стикання.
- Прибрано смугу бафів у HUD (кулдаун щита вже видно в слоті); значок щита намальовано векторно.
- Рівні 2 і 4 без ворогів: виправлено (дотик стіни блокував спавн-пойнт). Перевіряється автотестом --test.

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

- ✅ No file-level `USING_NS_CC`; namespaces by layer: `tanks::model`, `tanks::view::cocos` (scenes, HUD, widgets).
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

### After release: binary configs

- JSON stays the source format: it is what we edit and what Debug/editor builds read. Shipping builds read binarized configs packed into one or a few archives.
- Binary configs are read straight into memory objects: faster loading, smaller files (strings such as enum names and ids become ints, via the string store below).
- Localization stays text (not binarized). Today `StringsManager` loads `configs/strings/<language>.json` (flat `id → text`) through `ConfigReader` as a `cocos2d::ValueMap` per language and looks strings up by `std::string` id; it moves to `IConfigNode` and string-store ids in the loaders work.
- Every config type gets symmetric `Encode`/`Decode` (write/read) in the adapter layer, next to the JSON reader, so text and binary share one data model; a tool converts JSON → binary at build time, and tests check that JSON → binary → object gives the same object.
- File access goes through a small stream-based IO port (`std::istream`/`std::ostream`), not through filesystem calls spread across the code. Loaders get a stream, so the same loader works for files, packed archives and in-memory test data.

### Global string store (before the enemies/stats loaders)

- `model::NameId`-style handle: a small integer id for an interned string; compare and hash are int operations, `ToString()` gives a `std::string_view` into the pool.
- Two pools: persistent (config keys, ids, enum names; lives for the whole run) and transient (per level / per screen; cleared explicitly, ids from it are invalid after the clear).
- Everything else takes `std::string_view` instead of owning `std::string`; configs, loaders and the binary format store ids.

## ✅ HUD у стилі MOBA (базова версія)

- ✅ Верхня смуга рівня: назва, хвиля, вороги (шкала), рахунок, монети
- ✅ Нижня панель гравця: портрет, здоров'я, патрони, перезарядка, статистики, слоти вмінь (гаряча клавіша, іконка, кулдаун-шторка з секундами), іконки бафів зі шкалою під ними
- ✅ Візуальний щит на танку (бульбашка)
- ⬜ Анімація: щит мерехтить перед закінченням; підказка (tooltip) вміння
- ✅ Іконки вмінь і бафів у власному атласі (`textures/abilities`) замість запозичених кадрів

## 🔧 Зараз: інфраструктура (перед базою й скролінгом)

1. ✅ **Структура файлів.** `Classes/` розкласти по папках: `core/` (енуми, константи, конфіги), `game/objects/` (танк, куля, стіна...), `game/components/` (статистики, бафи, вміння), `game/ai/`, `game/` (фабрика, спавнер, рівні), `services/` (аудіо, профіль, апгрейди, рядки), `ui/` (віджети, HUD), `scenes/`. Інклюди відносно `Classes/`, `.vcxproj` і CMake оновлені.
2. ✅ **Юніт-тести** без зовнішніх залежностей (власний мінімальний раннер у `Classes/tests/`): статистики й бафи, пули спавнерів і обмеження, розбір і валідація рівнів, хвилі, апгрейди, збереження профілю.
3. ✅ **Автотести рівнів:** бот-гравець (`PlayerBot`: їде до найближчого ворога, стріляє, користується вміннями) проходить кожен рівень у прискореному часі без миші та клавіатури. Режим `TanksClone.exe --test`: юніт-тести, потім усі рівні, звіт у лог і код виходу (для CI).
4. 🔧 **Ресурси й текстури** (основне зроблено, див. «MVP-прибирання»):
   - ✅ Інвентаризація, прибрано залишки шаблону й запозичені кадри, задіяно невикористані звуки (старт рівня, руйнування стіни, вибух боса).
   - ✅ Структура `Resources/`, атласи генеруються скриптом, маніфест ресурсів рівня.
   - ⬜ **Роздільна здатність:** спрайти 32×32 масштабуються ×2 (піксельна графіка, `setAliasTexParameters`). Підготувати набори під 1×/2×/4× (піксельне масштабування без розмиття, цілі коефіцієнти), перевірити `SHOW_ALL` на різних співвідношеннях сторін, великі шрифти й HUD при 4K.
   - ⬜ **Справжній арт** замість згенерованих заглушок; спрайти бази (ціла / руїни), прев'ю ворогів для редактора.
   - 🔧 **Перевірка:** ✅ тест, що кожен кадр із конфігів є в атласі; ✅ `pack_atlases.py --check` і перевірка унікальності імен кадрів; ⬜ пошук невикористаних файлів і кадрів.
5. 🔧 **Resource manager (before Audio/Input ports and before scrolling).** Today file paths are raw strings in code (`AudioManager`: `"sounds/player_shot.wav"`, `GameScene`: `"configs/wall_brick.json"`, constants in `GameConstants.h`), so a platform, a mod or an update can't swap a file without a rebuild.
   - **Resource ids instead of paths:** code refers to a resource by a short id (`Sound::PlayerShot` → `"sfx.player_shot"`, `"wall.brick"`); `configs/resources.json` (extending today's manifest) maps id → path per resource type (sound, music, atlas, font, config). A platform or mod file overrides entries by id; a missing id logs an error and uses a placeholder.
   - **`ResourceManager` in `GameContext`:** typed handles (`SoundHandle`, `AtlasHandle`, `FontHandle`, `ConfigHandle`) that hold a reference while used, like Unreal soft/hard object references: soft = id only, hard = loaded and kept alive. The last handle released frees the resource (no manual purge, no use-after-free like the font atlas bug).
   - **Preload and async load:** a level asks for its resource set (from `ResourceManifest`: global + tileset + enemies + sounds); `LoadingScene` loads it with progress, async where the engine allows (textures), sync otherwise; scenes keep the set's handles alive and release them on exit.
   - **Ports:** the manager talks to the engine through a loader port (Cocos now: `TextureCache`, `SimpleAudioEngine`, `FileUtils`); Model code sees only ids and handles, so the core stays engine-free.
   - **Checks:** unit test that every id in code and configs resolves, every path exists, and no file is unused; `--test` covers missing-id fallback.
   - ✅ Step 1 done (2026-11-20): `model::ResourceTable` (id → path per kind, overrides by id), `ResourceManager` in `GameContext` (`ResolvePath`; unknown id is logged and gives an empty path, no placeholder file yet), sounds in `resources.json` under `"sounds"`, `AudioManager` on ids, unit tests (resolve, files exist, no unused sound file, overrides). 
   - ✅ Step 2 done (2026-11-20): `model::ResourceId` (a view of a literal name, no strings in `AudioManager`; `Sound` enum removed); ids live in `core/ResourceIds.h` (`ids::sounds`, `ids::fonts`, `ids::configs`); path constants gone from `GameConstants.h`, walls/bonuses/player/upgrades/fonts resolve through `ResourceManager`. A section of `resources.json` (and `global_atlases`/`tilesets`) may be the path of another config holding the same key (`configs/resources/*.json`). Atlas ids (enemy `atlas` fields, tilesets) still paths: they move with the handles step.
   - ✅ Step 2b done (2026-11-20): sounds are named in data. Tank configs take `sound_hit`, `sound_armor`, `sound_explode`; weapons and abilities take `sound`; any id of the sound table works (`ResourceManager::FindId`, ids view the table keys). Missing key = silent, unknown id = logged and silent. `AudioManager::Preload` loads the whole table. Not yet: bonus, wall and bullet-hit sounds (still `ids::sounds::*`, read from `ValueMap` configs).
   - ✅ Step 3 done (2026-11-20): handles. `model::RefCounted` (atomic count) + `model::Handle<T>`; `AtlasResource` (Cocos side) loads an atlas and unloads it with its last handle; `ResourceManager::AcquireAtlas(id)` shares one atlas per id; `AtlasSet` holds a group of handles (level, editor); global atlases are kept by the manager. `AtlasScope` is gone. Atlas ids (`atlas.common`, `atlas.enemy_sniper`, ...) in `configs/resources/atlases.json`; enemy `"atlas"` fields, tilesets and `global_atlases` use ids, not paths. Preload sets are step 4 below.
   - ✅ Step 4 done (2026-11-21): preload. `ResourceSet` (atlas and sound ids), `ResourceManifest::GetLevelResources` (tileset + enemy atlases + sounds named by the player's and the level's enemy configs), `ResourceLoadJob` (one resource per `Step()`, progress), `ResourceHolder` (handles for atlases and sounds; replaces `AtlasSet`), `SoundResource`. `LoadingScene::CreateForLevel` loads a campaign level's set with a progress bar, then opens `GameScene` with the holder; direct `GameScene` creation (tests, editor, custom levels) still loads synchronously. Async is step 5 below.
   - ✅ Step 5 done (2026-11-21): async. `AtlasResource::CreateAsync` decodes the texture through `TextureCache::addImageAsync` and finishes the atlas on the main thread; requests for one atlas share one load (`AtlasRegistry::Pending`); `ResourceLoadJob(Set, true)` starts every atlas at once and loads one sound per step, finishing when the last atlas arrives. `LoadingScene` uses it. Superseded by step 6.
   - ✅ Step 6 done (2026-11-21): loader thread. `model::WorkQueue` (engine-free): one worker thread that waits for jobs; each job is a Work function (file read, decode, parse; no engine access) and a Done function that `ResourceManager` runs on the main thread once per frame (`Pump`). Atlases now read and decode their PNG on that thread and are finished on the main thread (`addImage(Image*, key)` + `addSpriteFramesWithFileContent`; the plist is parsed on the main thread because the engine hides the dictionary call). Audio moved from `SimpleAudioEngine` to `experimental::AudioEngine` (`preload(path, callback)` is async, `uncache` unloads, volume is passed per `play2d`); `SoundResource::CreateAsync` uses it. `ResourceManager::ReadFileAsync` is the template for new kinds, e.g. binary configs: parse in the Work function, use the result in the Done function. Next: binary configs through the queue.
   - ✅ Step 7 done (2026-11-21): ports. `ports/` has `IAudioOutput` (play, stop all, preload with callback, unload), `IStorage` (named text files) and `IInputSource` (`model::KeyEvent`); Cocos adapters live in `platform/cocos/` (`CocosAudioOutput`, `CocosStorage`, `CocosKeyboardInput`). `GameContext` creates the adapters first (`PortAdapters` list) and hands them out through `GetAudioOutput()` / `GetStorage()` / `GetInputSource()`; a new platform replaces that list and the three getters. `AudioManager` and `SoundResource` play and preload through the audio port (`AudioManager::SetOutput` swaps in a fake for tests). The profile is now `profile.json` (`ProfileSerializer`, `JsonValue::ToString`) in the storage port, with a one-time conversion of the old `profile.plist` (`MigrateLegacyProfile`); the custom level goes through the storage port too. Key handling moved to engine-free `PlayerControls` (key events in, commands out), used by `KeyboardController`. Unit tests with fake audio and storage ports and fake key events. Loader flush (2026-11-21): `WorkQueue` has priorities (`WorkPriority::High/Normal/Low`), `Pump(budget)` (at least one job, then stops when the time is spent; `ResourceManager` uses `pump_budget_ms` from `resources.json`, 4 ms), `CancelAll()` (drops waiting, running and finished jobs without calling them) and `WaitIdle()`; `ResourceManager::CancelPendingLoads()` for a level change. Ready for streaming map chunks during gameplay; nothing streams yet. Still on the engine: `GameScene` / editor / test-runner keyboard listeners, `CommandType` (`ObjectEnums.h` includes cocos2d), config files read through `FileUtils` (`ConfigReader`).
   - ✅ Placeholders (2026-11-21): fixed a normal launch showing no sprites (the id table latched empty when the loading screen asked for a font before the configs were read). `model::ResourceStatusBook` records every resource asked for (unchecked / ok / missing); `ResourceManager::FindFrame` gives a magenta checker frame for a missing frame, a missing font falls back to the system font, a missing sound is silent; each is logged once and not retried until the texture set changes (`ReloadAtlases`). Tests: startup order, placeholder frame, and a run-wide "nothing missing" check at the end of every `--test`. ⬜ Not yet: placeholder for missing configs; unused files and frames.
   - Order: id table + `AudioManager` on ids → configs (`GameConstants.h` paths, wall configs) → handles with ref counting → preload sets in `LoadingScene` → async → then the Audio/Input/Storage ports (MVVM step 7).

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
