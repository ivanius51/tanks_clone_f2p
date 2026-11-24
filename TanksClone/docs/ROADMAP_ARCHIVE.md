# Roadmap archive

Finished work moved out of ROADMAP.md (2026-11-24). History, not needed at session start.

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

## ✅ Виправлено за відгуками

- Подвійний вибух при знищенні блока: тепер вибух лише блока; вибух кулі тільки для великих куль (explosion_on_hit) у точці стикання.
- Прибрано смугу бафів у HUD (кулдаун щита вже видно в слоті); значок щита намальовано векторно.
- Рівні 2 і 4 без ворогів: виправлено (дотик стіни блокував спавн-пойнт). Перевіряється автотестом --test.

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

