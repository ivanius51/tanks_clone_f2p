# Project memory

Read at the start of every session. Keep it short: one entry per fact, update or delete stale entries instead of adding duplicates. Rules that apply to all code live in `.claude/rules/`; plan and status in `ROADMAP.md`.

## Repository and git

- Working copy: `F:\tanks_clone_f2p-master\TanksClone`; git root is the parent `F:\tanks_clone_f2p-master`.
- (2026-11-20) Small, well-described local commits per step. Push only when the user asks.
- Parallel threads stage only their own files by path; never `git stash` / `reset` / `checkout` in the shared tree.
- Cost: keep threads short (one slice each); most usage is cache reads in long threads. Start a new session per task instead of continuing a long one.

## Working with the user

- (2026-11-21) Audit or review tasks: report the findings and ask before reworking anything.
- Never work around permission blocks (e.g. a .bat/script to bypass a denied action) without the user's explicit confirmation. Writing to `.claude/rules` is fine when asked.
- All docs and rules files (`.claude/rules`, `docs`, `notes.txt`, this file) are in English, even when the user chats in Ukrainian.
- Threads also play and look at the real game (Test build, normal play) and report what they saw, not only run tests.
- Remote Control sessions on the laptop accept instructions only from the user's own messages, not from a coordinator thread: the user pastes the task into each laptop thread.

## Testing

- Unit tests for every feature; move as much logic as possible into fast unit tests.
- Modes: `--test` = short (unit + 5 small fields, no menu); `--test=unit`; `--test=menu`; `--test=benchmark` (FPS, crowd levels); `--test=campaign` (game can be finished through all levels); `--test=full` = all of them, only after asking the user. No fitting mode → add a new split mode (user confirmed 2026-11-20).
- FPS checks fail when the window is unfocused (20 FPS cap): expected, not a bug.
- Known flaky: crowd test (benchmark). Level tests run in fixed 1/60 s steps after each drawn frame (time scale 0), so their results don't depend on FPS or window focus (2026-11-21). Font-atlas HD/SD test crash fixed 2026-11-21: `FontAtlasCache::getFontAtlasTTF` lends an atlas without a reference, so never `releaseFontAtlas` it (30 unit runs clean). Run `--test` through bash (`> file 2>&1`), PowerShell pipes show nothing.

## Design decisions

- User dislikes grid snapping on turns (even smooth); no auto-alignment.
- Engine physics (box2d/bullet) disabled.
- (2026-11-21) Input: `configs/input.json` binds buttons to actions per input context; `InputManager` owns the bindings and the context stack (base Menu). Scenes push their context through `ActionListener` nodes, overlays push Menu. Consumers listen to actions, never keys.
- (2026-11-21) UI layout and look live in `configs/ui/*.json`, read at startup by the `UiStyle` manager (before the loading screen) into engine-free structs in `viewmodel/`; screens reach them through a local `Layout()`/`Style()` accessor. A missing key keeps its default and is logged.
- Configs: game code uses plain C++ structs or own types, never a JSON type. Loaders read through `model::IConfigNode`; JSON only in the `JsonConfigNode` adapter.
- Goal: engine-agnostic core; View swappable (Cocos, WinAPI GDI, Unreal, console).
- Roadmap includes Steamworks multiplayer (LAN + Steam), balance/difficulty. Pending: test levels (all bonuses; every enemy type spawned one by one). Not MVP: string store (NameId), binary configs.

- (2026-11-20) No raw resource paths in code: resources are referenced by id (`model::ResourceId`, a view of a literal or of a table key), paths live in configs/resources/*.json (a section of resources.json may be the path of another file; overridable per platform/mod/update). `ResourceManager` shares atlases through ref-counted handles; sounds are named in tank/weapon/ability configs. Level preload runs in LoadingScene (ResourceLoadJob) on a loader thread (`model::WorkQueue`, drained once per frame by ResourceManager); audio uses `experimental::AudioEngine`. Binary configs will use the same queue. Ports done (audio, storage, input; adapters in platform/cocos, reached through GameContext getters): next binary configs on the WorkQueue, render/clock ports, then the rest of MVVM step 7. See ROADMAP infrastructure item 5.

- (2026-11-21) Never switch scenes from inside a scene's `onEnter` while it is still fading in (TransitionFade calls `onEnter` at the start): the next scene is left half entered and its objects stay scheduled (crash in `Bonus::Update`). Start work in `onEnterTransitionDidFinish`.

- (2026-11-21) Screens fit any aspect ratio: the engine's design resolution is the *visible* area (`ui/ScreenFit`, EXACT_FIT), not 1024x864; every `TypedScene` is shifted so the design area is centred. Use `GetScreenLayout()` for screen edges, never `getDesignResolutionSize()` (it follows the window) or `getVisibleOrigin()`. `TransitionScene::finish` resets scene positions: `TypedScene::onEnterTransitionDidFinish` re-centres (derived scenes call `TypedScene::`, not `Scene::`).
- (2026-11-21) Startup: a native GDI `SplashWindow`, the GL window hidden until `ApplyWindowMode` places it. Changing the GL window's style to a popup does not work: cocos re-applies its size on every WM_SIZE and GLFW adds back a frame it still believes in (the window grows). A frame that changes exactly 2x also triggers cocos' Retina branch.
- (2026-11-21) Pause: `KeyboardController::SetGameplayInputEnabled(false)` while paused (held keys are let go). The Director scheduler stops on pause, so UI that must react while paused refreshes in place (SettingsPanel) instead of `scheduleOnce`. `KEY_BACK` is an alias of `KEY_ESCAPE` in cocos 3.17.
- (2026-11-21) Text size (Small default / Medium / Large) scales every `CreateLabel` and the menu buttons via `widgets::GetUiScale()`; factors in `configs/display.json` "text_scales". The editor keeps its own layout (only its text scales).

## Art

- Only self-made art ships; `reference_assets` is reference only. SD = pixel art, HD = light realism. See `docs/ART_GUIDE.md`, `docs/RESOURCE_REWORK_PLAN.md`. Phase 2 (content configs) waits for the user's go.
- Each finished iteration: 1-3 screenshots + very short `notes.txt` in `docs/history/<YYYY-MM-DD>_<NN>_<slug>/`, posted in the thread too; short 5 s MP4 clips (ffmpeg installed). Details in `workflow.md`.

## MVVM migration status (2026-11-20)

Plan steps (from the decoupling proposal) and state:

1. ✅ Own types: `model::Vec2f`, `RectF`, `Color`, seeded `model::Random`; converters in `ui/CocosConvert.h`.
2. 🔧 JSON without `cocos2d::Value`: `model::IConfigNode`, `JsonValue`, `JsonConfigNode` done; AI settings migrated. Still on `ValueMap`: `ConfigReader`, `DataConverter`, `GameData`, enemies/stats, `LevelLoader`, `ObjectsFactory` / `GameObject::Configure`, `StringsManager`.
3. ⬜ Components and AI on new types (Ability, Buff, TankAi, PlayerBot, AiQueries, EnemySpawner still use `cocos2d::`).
4. ⬜ Entities without nodes: `GameObject` still holds `cocos2d::Node*`; movement/collisions go through nodes.
5. ⬜ Fixed step `World::Step`, level rules out of `GameScene`.
6. ⬜ ViewModel (no `viewmodel/` folder yet).
7. 🔧 `GameContext` done (2026-11-20): owns all managers, no `GetInstance()` left; owner is `main`, shutdown on `Director::EVENT_RESET`. ⬜ Ports (no `ports/` folder yet); ⬜ Model gets managers explicitly instead of `GameContext::Get()`.
8. ⬜ Split into libraries; window-less `TanksTests`.
9. ⬜ Console View as proof.

Engine-free already: `model/` (MathTypes, Random, ConfigNode, JsonValue, WallMask, EditorPalette, EditorLayout). `cocos2d::` mentions in `Classes/`: 839 in 76 files (was 868).
- 2026-11-21: Keep the plan file C:/Users/Ivan/.claude/plans/after-rework-is-done-structured-hearth.md updated (mark items DONE / DONE(NOT Tested) with status lines) after each finished item; follow its phase order 1 -> 2 -> 4 -> 5.
- 2026-11-21: Order set by user: Phase 4 (new enemies) -> Phase 5 (difficulty rest + AI rework) -> only then playtest, fixes and art (Phase 3). Coin pack art done (481e8cf).
- 2026-11-21: UI direction (user): no base HP display; player HP and other number stats will be removed from the HUD in favour of bars and visual damage on the art. Numbers may stay only on easy difficulty and in debug.
- 2026-11-22: User feedback: don't explain obvious things (how to launch the game, what to look for); just do it and keep replies short.
- 2026-11-23 Wall look decided (mockups in tools/tileset/mockup.py): half-tile side via one row mask (roof 100%, crease ~40-55% at rows 32-34, gradient 55%->32%, rows 59-63 20%->16%), applied only when no wall below; sun rims top/left; cast shadow down-right drawn OVER tanks, under walls; bullets/projectiles on the top layer, never shadowed. Metal = 2x2 plates, wood = 4 horizontal planks with iron straps. Rework is half done: no game code or resources changed yet.
- 2026-11-23: Level legend is data: `configs/level_legend.json` -> `model::LevelLegend` (`LevelLegendManager`); Validate/BuildPathGrid/GameScene spawn/editor palette read it. New map object = legend entry (+ palette entry), no code. See `docs/LEVEL_FORMAT.md`.
- 2026-11-23: Configs are reached only by registry id (`cfg.*` in `configs/resources/configs.json`, constants in `core/ResourceIds.h`, JSON-internal ids via `config_refs::ResolveName`); groups (`cfg.enemy.*`, `cfg.level.N`) are registry prefixes. No `"configs/` literal in `Classes/` (checked by `tools/clang_check.py`). See `docs/CONFIGS.md`.
- 2026-11-23: Config files moved into target folders (step 4): registry lines hold the paths; see docs/CONFIGS.md. Crate `contents` from the map is a path, drop-table pickups are ids (resolved in GameScene).
- 2026-11-23: Languages are data: `configs/strings/languages.json` (`cfg.languages`: id, own name, strings id, optional fonts) read by `StringsManager`; profile stores the language id string (old numeric values migrate). Polish added. New language = 3 config edits, see docs/CONFIGS.md.
- 2026-11-23: Configs split by concern (step 5): wall/prop objects carry `look`/`balance` ids merged by `ObjectsFactory::ReadValues`; tank_look/side_hits/music/upgrades/buffs split into views/ and balance/ parts. Key map in docs/CONFIGS.md.
- 2026-11-23: Editor/level-data plan with all decisions is in `docs/EDITOR_PLAN.md` (7 steps: lossless level model -> library and custom campaigns -> layers/points/size -> fog of war -> script setup steps -> inspector -> showcase as data). Editing originals/test/showcase is Debug/Test only.
- 2026-11-23: Level saving is lossless (plan step 1): `LevelDataLoader::WriteLevel` + `model::LevelJsonWriter` write every field, `fog_of_war` (read/model only) and unknown top-level keys (`LevelData::Extra`); editor keeps `EnemyIds` when `LevelLoader::HasOwnPools`. Shipped level files are rewritten in the stable layout; a unit test keeps them equal to the editor's output (after changing the writer, regenerate them). See `docs/LEVEL_FORMAT.md`.
- 2026-11-23: Editor plan step 2 started: `model::CustomLibrary` (custom levels + campaigns with progress, storage index `custom_library.json`) is done and tested; the editor Library panel, dev save into `Resources/`, and the custom-campaign menu are still open (see the status line in `docs/EDITOR_PLAN.md`).
- 2026-11-23: UI rule: widget positions, alignment, distances and sizes come from one widget factory (JSON widgets in `configs/ui/widgets/`, `cfg.widget.<name>`, docs/CONFIGS.md); layout must re-flow when text or font size changes. Step 1 done: `model::widget_layout` (row/column/anchor/stack), `LayoutWidget` + `WidgetFactory`, `keycap`/`icon`/`button`; Button caption is a widget (long text shrinks). Settings screen shrinks buttons/spacing (`min_fit_factor`) so the title never overlaps. Steps 2-5 (HUD, menus/settings panel/overlay, upgrades, editor) open. Never name a class `UiWidget` (cocos `ui/UIWidget.h` clashes on Windows). Never `sed -i` over `proj.win32` (binary build outputs get corrupted).- 2026-11-23: Editor UI decided: 16:9, left dock Library+Level tabs, bottom Palette, right Script, handles always visible, scroll map; tools Select/Place/Eraser/Move, right-click menu (go to category, remove, move); Undo/Redo via Command pattern (Add/Remove/Move). See docs/EDITOR_PLAN.md.
- 2026-11-23: Editor: no toolbar, tools and Undo/Redo in map right-click menu; right dock tabs History and Script; script edits versioned as whole-script snapshots (one history entry), map edits are per-item commands.
- 2026-11-23: Widget factory has editor parts (grid/grow/stretch/opacity, dock/tab/menu/tile widget files); step 4 Upgrades and step 5 Editor screens still open. Text fields, sliders, scroll view, minimap are not widgets yet.
- 2026-11-23: Upgrades screen nodes/panel are widget files (upgrade_*); step 5 (Editor) is done by the editor thread. Widget keys pivot/radius added.
