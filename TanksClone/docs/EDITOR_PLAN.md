# Level editor and level data plan

Agreed with the user on 2026-11-23, after the legend became data (commit b441c88). Self-contained: read this before starting any step.

Scope rule: editing original (campaign) levels, test levels, showcase and other test features exists in Debug and Test builds only. Custom levels and custom campaigns work in every build (storage port, `ports/IStorage`).

## Where things stand (facts found in the code)

- **Editor** (`scenes/EditorScene.*`, `model/EditorPalette`, `model/EditorLayout`, `viewmodel/EditorStyle`): edits one fixed 16x11 map (`GridRows` x `GridColumns` in `core/GameConstants.h`), one slot `custom_level.json`. Palette = `configs/editor/palette.json` (tab, tile, name) + `configs/level_legend.json` (picture and meaning of each character).
- **Saving loses data:** `LevelLoader::ToJson` writes only `name`, `music`, `tileset`, `rows` and the simple `enemies` form. It drops `spawners`, `waves`, `script`, `ai_features`. The editor also overwrites `Level.EnemyIds` with every enemy. Opening and saving an original level would wipe its waves and script.
- **Level files:** `configs/levels/campaign/level_1..6.json`, `configs/levels/test/test_*.json` (paths may have moved: the config-id-layer and split-configs sessions touch `levels/`; check `git log` before relying on a path). Campaign levels are found by probing `level_N` by index (`LevelLoader::GetBuiltinCount`/`BuiltinId`).
- **Reading:** `game/LevelDataLoader.cpp` reads a level through `model::IConfigNode`. `LevelData` (in `game/LevelLoader.h`) holds Name, Music, Tileset, Rows, simple enemy fields, `Spawners` (map char -> `SpawnRule`), `Waves`, `AiFeatures`, `Script`.
- **Scripts:** `model/ScriptPlayer.*` (engine-free). Steps today: wait, move_to / move_by, face, line_up, shoot, caption, until(event). Actors bind to spawn slots (`"a": "enemy#1"`). `game/SceneScript.cpp` wires it to `GameScene`.
- **Test levels and galleries:** `levels/test/test_*.json` hold only the static map. The states and effects live in separate gallery configs (`configs/test/look_gallery.json`, `hit_gallery.json`, `tanks_gallery.json`, `wreck_gallery.json`, listed in `configs/test/galleries.json`), rendered by `tests/Galleries.cpp`. A state has `label`, `health`, `hits: [[dx,dy]]`, `buff`, `tactic`; an effect has `label`, `effect`, `tile: [x,y]`, `shots`, optional `enemy`.
- **Showcase** (`tests/Showcase.cpp`, `--showcase <folder>`): screen walk plus screenshots and a clip; level choice, the boss level (`BossLevel()` trims waves), timings are code constants.
- **Legend** (done): `configs/level_legend.json` -> `model::LevelLegend` -> `LevelLegendManager`. Roles `empty, wall, prop, pickup, crate, player, spawner`; flags `pass` (open/breakable/blocked), `unique`, `is_base`, `default`. See `docs/LEVEL_FORMAT.md`.

## Design decisions

1. Dev/Test only: editing originals, test levels, showcase. Release: custom levels and custom campaigns only.
2. Config first, editor later. Every feature is first expressible in JSON and covered by unit tests; the editor edits that data. The editor does not have to expose every field with mouse tools.
3. Test and showcase cases become scripts (set health, hits from a side, buff, tactic, effect on an actor at a point). Galleries stay JSON for now; if the generic inspector can edit them cheaply, fine.
4. Animations stay JSON only (position fields), no timeline editor.
5. The Model simulates everything. Visibility (camera + fog) is decided in the ViewModel; the View draws the ViewModel's list. Fog never changes the simulation.
6. Fog of war is a per-level option: `fog_of_war { enabled, vision_tiles, show_initial_walls }`. Vision is a square (Chebyshev distance) of `vision_tiles`, one value per level, no per-difficulty override yet. Outside the square only the initial static map is drawn (walls and ground as at the start); no enemies, pickups, bullets, effects, destroyed walls.
7. Large maps show as many columns as the window ratio allows; only visible cells plus a margin are rendered. Fog removes the information advantage of wide windows.
8. Small one-screen maps: the authored area holds everything gameplay-relevant; extra columns on a wide window are plain ground (no spawners, no objects). OPEN: filler ground vs letterbox, the user has not decided; assume filler ground.
9. Level size comes from `rows` (width = row length, height = row count), no fixed 16x11. Keep reading old levels unchanged.
10. The level generator on the roadmap must write the same format.

## Steps

### Step 1: Lossless level model (no view change)

**Status (2026-11-23): done and verified.** `model::LevelJsonWriter` (stable layout, unknown keys kept in `LevelData::Extra`), `fog_of_war` read and written (`LevelData::Fog`), spawners/waves/script/ai_features kept on save. Unit tests: every shipped level survives load-write-load, shipped files are in the stable layout, unknown keys and fog survive, saving an original keeps its pools, waves and script. Left for later steps: `objects` and `points` keys (step 3).

- Engine-free writer in `model/` (`LevelWriter` or a `LevelData` serializer; `LevelData` itself may move to `model/`). Writes every field: name, music, tileset, rows, simple enemies, spawners, waves, `ai_features`, `script`, `fog_of_war`, and later `objects`, `points`.
- Unknown keys read from a file are kept and written back (store the leftover nodes in `LevelData`), so a newer data field is never destroyed by an older editor.
- Stable key order, tabs, one row per line, so git diffs stay small.
- `LevelLoader::ToJson` becomes the thin wrapper; the editor stops overwriting `EnemyIds` when the level has its own pools.
- Add `fog_of_war` to the read side (`LevelDataLoader`) and the model struct; render side comes in step 4.
- Tests (unit, fast): load every shipped level (campaign, test, custom), write, parse again, compare all fields; a level with unknown keys keeps them; bad rows still rejected by the legend validator; editor save of an original no longer drops waves/script.
- Update `docs/LEVEL_FORMAT.md` (keys table).

### Step 2: Level library and saving

**Status (2026-11-23):** done: the model of custom levels and custom campaigns (`model::CustomLibrary`, index file `custom_library.json` through `ports/IStorage`, one `level_<id>.json` per level, progress and coins per campaign) with unit tests. Not done: the Library panel in the editor, Save into the source tree in Debug/Test, the "Custom campaigns" menu entry and campaign play, the Campaign sequence editor, and the ordered campaign list file (the built-in order still comes from the `cfg.level.N` registry group).

- **Done 2026-11-24:** `configs/levels/campaign.json` (`cfg.campaign`, read by `model::CampaignList`) is the shipped campaign order; `LevelLoader` follows it. Custom campaigns already use the same idea in `custom_library.json`.
- Ordered campaign list file (for example `levels/campaign.json`: `{ "levels": ["level_1", ...] }`) replaces probing by index. Same format for custom campaigns.
- Editor: Library panel (Campaign / Custom / Test [dev only]), Open, New level, Save, Play. In Debug/Test builds Save on an original writes into the source `Resources/` tree (a dev config gives the root; never in Release); otherwise saves a custom copy through the storage port.
- Custom campaign sequences: a list of custom level ids with per-campaign progress and coins, saved through the storage port, playable from a menu entry "Custom campaigns" in every build. The editor has a "Campaign sequence" editor (order, add from Custom, save).
- Tests: campaign list load, sequence save/load, release build has no source-tree write path.

### Step 3: Layers, points and size

**Status (2026-11-26):** done: any map size from rows; `model::LevelLayers` (`objects` and `points` keys read, written, round-trip tested); editor: Width/Height steppers (undoable `ResizeMapCommand`, layers cropped), Layer stepper Ground/Points (click toggles a named point, yellow markers), Level type (tileset) stepper. Layer stepper cycles Ground / Objects / Points / Floor (2026-11-26): Objects places the selected palette entry's config (Eraser removes); Points: click adds or selects, the Point stepper renames from `point_names`, Eraser deletes; Floor paints roads and mud (`ground` key, legend `ground` list). Not done: undo for points, objects and floor edits and for the layer crop.

- Layer 1 Ground: `rows` as today (legend characters), any size.
- Layer 2 Objects (optional, sparse): `[ { "config": "cfg.x", "cell": [x,y], "name": "p2_start" } ]`. First uses: starts for players 2 and 3 (multiplayer on the roadmap), single props, script actors.
- Layer 3 Points (optional): `"points": { "a": [4,2], "show": [7,5] }`. Spawners, script actors and test setups link to a point by name, so moving a point moves every link.
- Old levels load unchanged; characters in `rows` stay shorthand for objects.
- Editor: Ground / Objects / Points tabs, map resize (add or remove rows and columns), points shown as dashed markers.
- Tests: size from rows, objects and points round-trip, link resolution by name, a missing point is reported.

### Step 4: Fog of war and wide screens

- ViewModel builds the draw list from camera rect and fog square; View only draws it. Static start-state layer for fogged cells.
- Camera for large maps follows the player in any direction; virtualization renders only visible cells plus a margin (see ROADMAP large-map items).
- Editor: checkbox "Fog of war" and a `vision_tiles` number field.
- Tests: no entity outside the vision square is in the draw list; with fog off everything in the camera rect is; bot still wins fogged levels.

### Step 5: Script setup steps (config only)

Add to `model::ScriptStepKind`, all engine-free and unit-tested with the fake world:
- Test and showcase setups: `spawn` (enemy id at a point), `set_health` (fraction), `hit` (side, count), `buff`, `tactic`, `play_effect` (at a point, with shots as in the hit gallery).
- Needed by other planned features: `spawn_enemy` / `summon` (boss phases), `set_ai_features`, pause or release AI (tutorial demos), `give_pickup`, `play_sound` / music, `camera_focus` (large maps), `set_difficulty`, `end_level` (win/lose).
- Waits on events: `kills >= N`, `pickup.taken`, `player.in_area`, `actor.destroyed`.
- Tests mirror the existing ScriptPlayer tests.

### Step 6: Editor inspector and script list

- Generic fields declared in data (number, toggle, dropdown, text), no per-feature UI code. First users: level meta (name, tileset, music, `ai_features`, `fog_of_war`), enemy pools, waves list.
- Script panel: step list (add, remove, reorder, edit values); a node graph can replace it later over the same data.
- "Run on selected point": run the script on a chosen enemy at a chosen point in a preview (`GameScene::CreateForTest`).

### Step 7: Showcase and test cases as data

- `showcase.json` (shot list: level, time, wave trim, script) executed by `tests/Showcase.cpp`; test levels and gallery-style cases written as scripts from step 5. Optional node graph editor.

## Mockups shown in the planning session

(Not in the repo; redo them before steps 2, 3 and 6 are built.)

1. **Screen width:** the same 16x11 level on 4:3 (exactly the authored area) and 16:9 (plain-ground extra columns, authored area dashed); large map: camera follows, as many columns as the ratio allows.
2. **Layers and points:** three stacked layers (Ground rows, Objects list, Points) with arrows from points to spawner / player-2 start / script step; old levels have no layers 2 and 3.
3. **Dev editor:** badge "Dev / Test build only"; left Library tree (Campaign, Custom, Test [dev], New level); centre map with tabs Ground / Objects / Points and size; right Level inspector (name, tileset, music, ai features, waves); bottom Script panel (step cards spawn / set_health / hit / caption) with "Run on selected point".
4. **Custom campaigns:** menu list of custom campaigns with Play; editor "Campaign sequence" list with move up/down, add level, save.

## Risks

- Step 1 must not reorder or reformat shipped levels in a way that hides real changes: write the stable format once, commit the reformatted files separately.
- Writing into `Resources/` must be impossible in Release (compile-time or build-config gated).
- The id-layer and split-configs sessions are moving paths under `configs/`; recheck paths before step 1 and use resource ids, not raw paths.
- Fog culling must stay in the ViewModel; do not add engine visibility checks inside the Model.
- Cocos widgets for lists and fields do not exist yet; the inspector (step 6) needs a small reusable widget set first.

## Open questions

- Small one-screen maps on wide windows: filler ground or letterbox (default: filler).
- Whether galleries get the inspector in step 7 or stay hand-edited JSON.


## Editor UI design (decided 2026-11-23, 16:9 main aspect)

Layout (mockup approved): left dock = tabs Library + Level, bottom dock = Palette, right dock = Script. The map fills the rest and is a pan/scroll view (scrollbars, wheel, drag, minimap) because maps will have different sizes. Every dock has a collapse handle in its own header that stays visible when collapsed (a thin strip). Docks are widgets from the widget factory; thresholds and sizes live in `configs/ui/editor.json`.
Editor panels are JSON (`configs/ui/widgets/editor_*.json`; bodies of Library, Level, Palette, History, Script); `EditorScene` only binds data by widget id. Widget features added for it: child `show_if` (flag set with `SetFlag`), `item` (repeated list items via `SetListCount`), `texts` / `text_keys` (labels inside a sub-widget), grid `columns: 0` (auto-wrap), row rule `equal_children`, and click/enabled state on `LayoutWidget` (`SetClickHandler`, `BindClick`, `SetEnabled`). Every stepper (zoom, size, layer, floor, level type) is the shared `stepper` widget with `<` `>`.


Tools: Select cursor (picking an object selects it and jumps the Palette to its category and item), Place (current palette item), Eraser (delete or change an object), Move.
Right click on an object opens a small context menu: Go to category, Remove, Move (Move = pick up, then click the target cell; click-and-hold drag can be added later).

Undo/Redo: Command pattern in the engine-free model. `EditorCommand` (Apply, Revert) with `EditorHistory` (undo and redo stacks; a new command clears redo). Commands are described by item, position and action: Add, Remove, Move (= remove at the old position plus add at the new one, one undo step), plus level property changes. Every editor action goes through a command, never straight into `LevelData`. Unit tests cover apply/revert/redo and the Move pair.

### Editor UI update (2026-11-23)

No toolbar. Tools (Select, Place, Eraser, Move) and Undo/Redo are in the right-click menu of the map; right-clicking an object adds Go to category, Remove, Move at the top. The right dock has two tabs: History (Undo, Redo buttons; click an entry to jump to that step) and Script.

Script edits are not command-based: each script change saves a whole-script snapshot (versioning). History shows the snapshot as one entry ("Script changed") and Undo/Redo restore the whole script text. Map edits stay per-item commands (Add, Remove, Move).
