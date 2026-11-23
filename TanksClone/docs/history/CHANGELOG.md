# Історія змін TanksClone

Знімки й кліпи кожної ітерації лежать у `docs/history/<папка>/` (див. правило в `.claude/rules/workflow.md`).
Історію комітів git тут не використано: у робочій копії немає `.git`. Дати ранніх етапів невідомі, вони позначені «раніше».

## Ранні ітерації (раніше)

Знімки цих можливостей з поточної збірки: [00_earlier-features](00_earlier-features/).

- **00.1 Рефакторинг шаблону.** Простір імен `tanks`, `constexpr` замість макросів, `unique_ptr` у фабриці, кросплатформний `ConfigReader`, фабрика об'єктів з JSON (`createFromConfig`, `move_animation`), виправлено UB, `nullptr` у `CommandsManager`, доставку команд клавіатури.
- **00.2 Гра, меню, редактор.** Екрани: завантаження, меню, гра, апгрейди, налаштування, редактор. Кулі, стіни, бонуси (патрони, ремонт), вибухи, звуки. AI ворогів (`Wander`, `Hunter`), спавн-пойнти, рівні 16×11 у JSON, збереження профілю, магазин апгрейдів.
- **00.3 Розширювані вороги.** Статистики, бафи, вміння як окремі сутності; вороги як файли `configs/enemies/<Id>.json` з класами Light / Medium / Heavy / Boss; спавнери `E`, `1`-`9` з пулами й обмеженнями, хвилі, бос на рівні 6.
- **00.4 HUD у стилі MOBA.** Верхня смуга рівня, панель гравця (портрет, здоров'я, патрони, перезарядка, статистики, слоти вмінь із кулдауном), бульбашка щита.
- **00.5 Інфраструктура тестів.** Структура `Classes/` по папках, юніт-тести без зовнішніх залежностей, бот `PlayerBot`, режим `--test` зі звітом і кодом виходу. CCD куль і підкроки руху, ліміт FPS у налаштуваннях.

## 2026-11-19

- **01 MVP-прибирання** ([папка](2026-11-19_01_mvp-cleanup/)): прибрано залишки шаблону Cocos (HelloWorld, CloseNormal, шаблонні коментарі, іконка); власна згенерована pixel-art графіка замість запозиченої; атласи з `tools/pack_atlases.py`; маніфест ресурсів (рівень вантажить лише свій тайлсет і своїх ворогів); тайлсети field і desert; синтезовані звуки; структура `Resources/`; `dist/Debug`, `dist/Release`, архів релізу; `.gitignore`; Release без фізики рушія (Box2D, bullet).
- **02 Шрифти, українська, збірка Test** ([папка](2026-11-19_02_fonts-ukrainian-test-build/)): шрифти OFL Press Start 2P і PT Sans з ліцензіями; українська локалізація; третя конфігурація Test (оптимізації Release + `--test` і F3).
- **03 Кулемет і стіни з клітинок** ([папка](2026-11-19_03_machine-gun-brick-chipping/)): друга зброя (кулемет, Q / Tab, без снарядів); стіни 4×4 клітинки, куля вибиває з боку удару, прохід можна прострілити й проїхати; автотести меню (натискання кнопок на всіх екранах, тимчасовий профіль); розділ «Баланс і складність» у ROADMAP.
- **04 Застряглі танки, FPS, Steam у планах** ([папка](2026-11-19_04_stuck-tanks-fps-fixes/)): спавн не в танк, роз'їзд танків, що перекриваються; ліміт FPS працює в грі (меню 60, vsync через WGL, таймер 1 мс); автотести натовпу ворогів і вимірювання FPS; розділ «Мережева гра (Steamworks)» у ROADMAP.
- **06 Налаштування екрана** ([папка](2026-11-20_06_display-settings/), 2026-11-20): у вікні / на весь екран без рамки, розмір вікна (Авто, 100-200 %), пікселі чіткі або згладжені, вертикальна синхронізація окремо від ліміту FPS, гучність ефектів; усе зберігається в профілі й застосовується одразу; автотести меню перевіряють розмір вікна й застосування налаштувань. Кліпи тепер MP4 (ffmpeg).
- **05 Без вирівнювання по сітці** ([папка](2026-11-19_05_no-grid-snap-corner-assist/)): поворот більше не зсуває танк; ковзання по кутах (`corner_assist` у JSON); AI ворогів повертає біля центру клітинки сам; бот не стоїть на точці спавну; режим `--showcase` (знімки екранів і кліп) і `tools/make_gif.py`.
- **07 Позиція вікна, HD/SD** ([папка](2026-11-20_07_window-position-hd/), 2026-11-20): вікно відкривається там, де його залишили; перемикач текстур HD/SD у налаштуваннях; виправлено збірку; 3 тести FPS падали лише через вікно без фокусу (ліміт 20 FPS).
- **08 Безшовна земля** ([папка](2026-11-20_08_seamless-ground/), 2026-11-20): тайли землі field і desert (SD і HD) безшовні: шум із загортанням, спільний для всіх 4 варіантів, деталі переходять через край; `tools/check_tileable.py` перевіряє шви; правило в `docs/ART_GUIDE.md`; атласи перепаковано.
- **09 Панелі редактора** ([папка](2026-11-20_09_editor-panels/), 2026-11-20): панель редактора розділено на палітру з вкладками (стіни, юніти, бонуси, інструменти; з `configs/editor/palette.json`) і окрему панель дій (Зберегти, Перевірити, Очистити, Назад, лічильник ворогів); розкладку рахує `model::EditorLayout` без рушія; юніт-тести й автотести `--test`.
- **10 Шрифти після HD/SD** ([папка](2026-11-20_10_font-atlas-scale/), 2026-11-20): виправлено зіпсований текст меню після перемикання HD/SD (і завжди в збірці Test): кеш атласів шрифтів очищується, мітки беруть новий атлас за подією `ContentScaleChangedEvent`; тест SD→HD→SD.
- **11 Плейтест рівня 1** ([папка](2026-11-20_11_playtest-level1/), 2026-11-20): ручна гра рівня 1 у збірці Test, знімки й нотатки про знайдені дивацтва.
- **12 Loading screen** ([folder](2026-11-21_01_loading-screen/), 2026-11-21): resource manager work: ids instead of paths, ref-counted handles, preload sets, loader thread (`WorkQueue`), audio on `AudioEngine`; campaign levels now open through a loading screen with a progress bar and a progress log.
- **13 Ports** ([folder](2026-11-21_02_ports/), 2026-11-21): audio, storage and input behind ports (`ports/`, Cocos adapters in `platform/cocos/`); profile saved as `profile.json` with a one-time conversion of the old plist; key handling in engine-free `PlayerControls`; tests with fake ports.
- **14 Resource placeholders** ([folder](2026-11-21_03_resource-placeholders/), 2026-11-21): fixed the empty resource table on a normal launch (no sprites, no player); missing frames, fonts and sounds get placeholders and are recorded in a status book; tests for startup order and a run-wide missing-resource check.
- **15 Pause, input, text size, aspect ratios, splash** ([folder](2026-11-21_04_pause-input-aspect/), 2026-11-21): hold to fire; true pause with in-game options; Esc/Back as back; text size setting; screens fill 4:3, 16:9, 16:10 and phones with a floating HUD; startup splash window, the game window appears already placed.
- **16 Input actions, UI configs** ([folder](2026-11-21_05_input-actions-ui-configs/), 2026-11-21): keys -> actions per input context from `configs/input.json`; UI layout, colors, text sizes, HUD anchors and the splash in `configs/ui/*.json`; scene fades without the jump to the left; splash minimum time; level tests in fixed steps.
- **17 Menu volume, shield hits, upgrades scroll** ([folder](2026-11-21_06_menu-volume-shield-scroll/), 2026-11-21): Left/Right step a focused setting; volume in 5% steps; a bullet hitting a shield vanishes with a ripple; the upgrades list scrolls.
- **18 Upgrade branches** ([folder](2026-11-21_07_upgrade-branches/), 2026-11-21): per-weapon upgrade lines (unlock, reload, damage) with prerequisites; weapons open after progress on other lines; locked rows show what they need.
- **19 Upgrade tree blueprint** ([folder](2026-11-21_08_upgrade-tree/), 2026-11-21): the shop is a blueprint of the tank, top and side views; weapon tabs above the turret with a box of the picked weapon's upgrades; round nodes with level rings linked to tank parts; new move speed and max HP upgrades; arrow navigation.
- **20 Achievements, hidden upgrades** ([folder](2026-11-21_09_achievements/), 2026-11-21): counters in the profile, data-driven achievements shown on the result screen, upgrades hidden until an achievement (ammo belt after 50 kills).
- **21 Death penalty, paid repair** ([folder](2026-11-21_10_death-penalty/), 2026-11-21): each loss on a level shrinks starting health and ammo (floor 40%) until the level is won or repaired for coins from the result screen; deaths per level saved in the profile.
- **22 Difficulty modes** ([folder](2026-11-21_11_difficulty/), 2026-11-21): easy/normal/hard/nightmare from config scale enemy health, damage, spawn interval and coins; chosen in Settings, saved in the profile; the death penalty is off on easy.
- **23 Enemy drops** ([folder](2026-11-21_12_drops/), 2026-11-21): pseudo-random drop tables with pity, small repair / ammo / coin pack pickups that blink out, drop chance per difficulty.
- **24 Coin pack art** ([folder](2026-11-21_13_coin-art/), 2026-11-21): the coin pack pickup has its own gold coin sprite (SD and HD) instead of the tinted ammo icon.
- **25 Wheeled BTR** ([folder](2026-11-21_14_wheeled-btr/), 2026-11-21): new "wheels" movement that turns along an arc, fast armored car enemy with a machine gun burst, in levels 2 and 4.
- **26 Ram vehicle** ([folder](2026-11-21_15_ram/), 2026-11-21): Charge ability rams an enemy in line, contact damage, then a stun; ram enemy in levels 3 and 5.
- **27 Splitter** ([folder](2026-11-21_16_splitter/), 2026-11-21): enemies can split into smaller tanks on death (split_into / split_count); splitter enemy in level 4.
- **28 Shield bearer** ([folder](2026-11-21_17_shield-bearer/), 2026-11-21): hits are classified front / side / back; frontal_shield and rear_damage_scale config keys; shield bearer enemy in level 3.
- **29 Mortar** ([folder](2026-11-21_18_mortar/), 2026-11-21): lobbed weapons fly over walls and blast an area on landing; mortar enemy in level 5.
- **30 Engineer** ([folder](2026-11-21_19_engineer/), 2026-11-21): Repair ability heals the most hurt allies nearby on its own; engineer enemy in level 4.
- **31 Minelayer** ([folder](2026-11-21_20_minelayer/), 2026-11-21): LayMine ability drops the mine bullet behind a driving tank; minelayer enemy in level 5.
- **32 Cloaker** ([folder](2026-11-21_21_cloaker/), 2026-11-21): Cloak ability fades a tank nearly invisible and back, a hit reveals it; cloaker enemy in level 5.
- **33 Boss phases** ([folder](2026-11-21_22_boss_phases/), 2026-11-21): "phases" in tank configs (abilities, buffs, escorts at a health share); the boss enrages, lays mines and calls two escorts below half health.
- **34 Difficulty multipliers** ([folder](2026-11-21_23_difficulty_multipliers/), 2026-11-21): bullet speed, enemy reload and spawn count per difficulty; a best score for each mode.
- **35 Difficulty AI** ([folder](2026-11-21_24_difficulty_ai/), 2026-11-21): per-mode enemy ability cooldowns, AI reaction, fire rate, aim tolerance and friendly fire.
- **36 AI tactics** ([folder](2026-11-21_25_ai_tactics/), 2026-11-21): enemies lead their shots, dodge incoming bullets, flank and retreat when hurt (model::AiTactics).
- 2026-11-21 #37 AI feature switches per profile, enemy, difficulty and level; enemies guard pickups and play as a group (pin + flank)
- 2026-11-21 #38 Player base tile 'H' (losing it loses the level), enemies attack it; anti-camping hunts a player who stays put
- 2026-11-21 #39 AI cover (step out of the line of fire while reloading) and fake turns
- 2026-11-21 #40 AI debug view: F4 shows each enemy's current tactic over the tank
- 2026-11-21 #40b AI debug view also shows each enemy's HP and when it is reloading
- 2026-11-22 #40c AI debug view: over each tank only '#n tactic'; a side panel lists every enemy's hp, armor, damage, speed, phase, reload, buffs and ability cooldowns
- 2026-11-22 #41 Tank look: tactic lamp, staged cracks, hit-face holes, light/dark smoke, dirt when slowed; look gallery test level
- 2026-11-22 #42 Tank art in two layers: hull and turret (turret on top), ready for separate turret art
- 2026-11-22 #43 Hit effects per collision (effects.json, sparks per material) and a Debug menu with galleries: tank looks, hit effects, all tanks
- 2026-11-22 #44 Separate left/right track layers animated by speed (outer track only when turning); bigger hit sparks
- 2026-11-22 #45 Small effects: muzzle flash, bullet fizzle at range end, dust behind moving tanks, pickup sparkle
- 2026-11-22 #46 Side hits: stacking slow, a big side loss stops the tank with track sparks (side_hits.json, model::DamageState)
- 2026-11-22 #47 Supply crates: level bonuses sit in crates that must be shot open; C tile crate drops a random bonus
- 2026-11-22 #48 Fuel barrel (F tile): explodes when shot and hurts every tank nearby; loader accepts C and F tiles
- 2026-11-22 #49 Script player (model only): level script steps wait, move_to, face, shoot, caption, until; unit tested, not in the game yet
- 2026-11-22 #50 Bullet clash (bullet_health) and friendly_fire_share: difficulty switches, off by default
- 2026-11-22 #50 follow-up: bullet clash turned on for Easy and Normal
- 2026-11-22 #51 Scripted scene: level 1 friendly-fire demo driven by the level script (SceneScript)
- 2026-11-22 #52 Kill credit only for the player's kills; broken wall cells fly out as debris
- 2026-11-22 #53 Tank damage overlays clipped to the tank sprite outline
- 2026-11-22 #54 Tank destroy animation: burning wreck with smoke, hull pieces fly out
- 2026-11-22 #55 Tank corpse: the wreck stays as an obstacle, breaks after 2 hits, gone after 20 s
- 2026-11-22 #56 unique corpses: per-vehicle wreck art without turret, turret flies off, corpse HP from overkill
- 2026-11-22 #57 wreck gallery; switch galleries from the pause menu
- 2026-11-22 #58 upgrades weapon fan-out on turret, full-window background; enemies counter counts all defeated
- 2026-11-22 #59 machine gun overheat; turn time per quarter turn, slower turns
- 2026-11-22 #60 gamepad: XInput pads feed input.json bindings (stick, triggers)
- 2026-11-22 #61 editor fixes: fullscreen click offset, base texture, Game Over with a tank/base reason
- 2026-11-22 #62 editor objects: wreck, mine, barrel, random box in the palette and level maps
- 2026-11-22 #63 smoke: black soot at low HP, separate light wisps over wrecks
- 2026-11-22 #64 walls and dirt: every part breaks away, seamless cells, denser mud
- 2026-11-22 #65 wall rims, faster debris, muzzle flash over the barrel with a flare
- 2026-11-22 #66 soft stepped wall edges, editor barrel/crate icons, muzzle flash tuned
- 2026-11-22 #67 wall edges 50/70/90 with an overlap rim outside the block
- 2026-11-22 #68 shot glow on the hull, hit effects on the struck face, walls keep one look when hit
- 2026-11-22 #69 muzzle flash per weapon and barrel count; plain walls break without a blast
- 2026-11-22 #70 track ruts and tire marks on the ground
- 2026-11-22 #71 wreck art fix, per-checkout engine build folder, PCH, /W4 clean
- 2026-11-22 #72 gamepad in pause menu, left/right on all settings, clang-cl strict check
- 2026-11-22 #73 cloak-win-grace: full cloak, split parts not counted, 15 s bonus time after a win (Back skips)
- 2026-11-22 #74 player-kills-only: enemies killed by enemies are replaced, not counted
- 2026-11-22 #75 gamepad-hints: HUD and menu hints follow the last input device
- 2026-11-22 #76 ai-paths-size: square tank collision, smaller enemies, AI clears breakables, grid pathfinding, blocked-level check
- 2026-11-22 #77 Menu click/move sounds
- 2026-11-22 #78 WAV decoder fixes silent sound; output device option; --test=audio
- 2026-11-22 #79 Background music (calm/intense, crossfade); music volume; tools menu fixes
- 2026-11-22 #80 Synthwave music style; calm_7; four tracks restyled
- 2026-11-22 #81 Cyberpunk music with ambience, softer drums; drumless menu music; quieter music when paused
- 2026-11-22 #82 Softer drums; per-level music; arranged level_1 track
- 2026-11-22 #83 CC0 music (18 tracks), longer quieter generated music, menu fade and volume cap, credits data, tileset pilot tool
- 2026-11-22 #84 Readable music file names; texture look mockups
- 2026-11-23 #85 Wall look mockups (tool only): side mask rule, cast shadow over tanks, tilted tanks
- 2026-11-23 #86 Configs split: look/balance parts for walls and props, side_hit_sparks, tank_corpse, music_rules, upgrade_tree, buff_icons
- 2026-11-23 #87 Polish language; languages are a config list (id, own name, strings, fonts), profile stores the id, missing keys/glyphs logged
- 2026-11-23 #87 Weapons as own configs; upgrade tabs and HUD icons come from them
- 2026-11-23 #88 HUD weapon switch button: separate icon button (arrows + key hint) on the weapon slot, clickable
- 2026-11-23 #89 Widget factory step 1: JSON widget files (button, keycap, icon), engine-free row/column/anchor/stack layout that re-flows on text changes; settings list fits the screen, title no longer overlaps the first button
- 2026-11-23 #90 Widget factory step 2 (HUD): hotkeys use the keycap widget, switch bar is a widget (ring + keycap); test runs have a wall-clock cap (--max-time, 900 s default)
- 2026-11-23 #91 Widget factory step 3: overlay, control hints, settings panel and main menu are widget files (centred groups that fit the screen); appended nodes, hidden children and wrapping labels
- 2026-11-23 #92 Widget factory editor parts: grid layout, grow/stretch/opacity keys, part recolouring; dock, tab, list item, context menu, tile widget files (editor itself not started)
- 2026-11-23 #93 Editor slice 2: dock layout model, docks as widget files with collapse handles, scrolling map (wheel, drag, scrollbars, minimap, virtualized cells)
- 2026-11-23 #94 Editor slice 3: tools, map right-click menu, History tab with undo/redo/jump
- 2026-11-23 #93 Widget factory step 4 (Upgrades): nodes and side panel are widget files; new pivot and radius keys; tree, links, focus and shop logic stay in code
