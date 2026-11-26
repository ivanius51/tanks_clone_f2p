# TanksClone Roadmap

Finished sections are in `docs/ROADMAP_ARCHIVE.md` (not read at session start).

Statuses: ✅ done · 🔧 in progress · ⬜ planned

## ⬜ Network multiplayer (Steamworks)

Depends on the engine-independent core (MVVM) and deterministic fixed-step simulation; networking is not feasible without them.

- ⬜ **Network port** (Model interface, independent of Steam and the engine): connect, send player commands, receive state, and emit player-joined/player-left events. Implementations: local (tests and same-PC play), LAN (UDP), Steam.
- ⬜ **Synchronization model:** start with deterministic lockstep (everyone executes the same commands with the same seed; only commands are sent over the network; compare state hashes every N steps). Alternative for PvP: server-authoritative (host simulates; clients receive and interpolate state snapshots).
- ⬜ **Steamworks SDK:** lobbies (`ISteamMatchmaking`), friend invites and joining through the overlay, Steam Networking Sockets with relay (no port forwarding required). Include the SDK only in the Steam build (a separate configuration); other builds work without it.
- ⬜ **LAN:** discover games on the local network (broadcast), without Steam.
- ⬜ **Modes:** start with co-op (2 players against waves, shared base), then PvP (teams, base capture).
- ⬜ **Optional:** Steam achievements, Steam Cloud profile saves, leaderboards.
- ⬜ **Tests:** run two simulations with a local network port and bots; verify their states match (hash) after N minutes of play; simulate latency and packet loss.

## ⬜ Balance and difficulty

- ⬜ **Upgrade prices:** currently linear (`base_cost + cost_step × level`). Calculate income per level (coins for enemies + victory bonus) and tune prices so a campaign run lets players buy about 60–70% of upgrades; consider geometric price growth.
- ⬜ **Enemy difficulty curve:** table mapping level to enemy count, `max_alive`, interval, and Light/Medium/Heavy/Boss proportions; gradual scaling, breather levels, and a boss every Nth level. Use a bot to measure completion time and HP lost per level.
- ⬜ **Ammunition economy:** with 20 starting shells and `A` bonuses, players should not get stuck without shells; the machine gun is the fallback. Tune the chance and amount of ammunition from bonuses and enemy drops.
- ⬜ **Difficulty modes** (easy / normal / hard): JSON multipliers for enemy health and damage, bullet speed, enemy count, and coins (`configs/difficulty.json`); add a settings selection and a separate record for each mode.
- ⬜ **Weapon and wall balance:** compare cannon and machine-gun DPS, shots needed to break brick and wood, and whether the machine gun makes the cannon redundant.
- ⬜ **Automated balance checks:** have a bot play each difficulty mode N times with different seeds; report win rate, average time, remaining HP, and ammunition.

## ⬜ Quality and standards

- ✅ Rules in `.claude/rules/` (naming, comments §19, architecture, workflow). Follow them for new code; update comments to match only in files already being edited.
- ⬜ **Rename constants** from `UPPER_SNAKE_CASE` to `PascalCase` (for example, `PLAYER_NAME` → `PlayerName`, `TILE_SIZE` → `TileSize`).
- ⬜ Self-review (code review) before completing each task, using the checklist in `workflow.md`.
- ⬜ Add a `/** */` doc comment for every main class.
- ⬜ **Data-driven:** everything tunable in JSON (input actions, `configs/ui`, balance, resource ids, tables, defaults, strings): audit and one work order in `docs/DATA_DRIVEN_PLAN.md`.

## ✅ Engine prebuilt + CMake clang-cl (2026-11-25)
- Engine 3.17.2 in `cocos2d/`, built once by `tools/engine_libs.ps1` into `libs/<Debug|Release>/`; the game vcxproj and CMake only link these binaries.
- CMake presets for clang-cl (Debug, Release, Test) via `cmake/PrebuiltEngine.cmake`; `-U__SSE__` keeps the Node layout equal to the MSVC engine.

## ⬜ Engine-independent core (MVVM)

Goal: make the logic work with any renderer (Cocos, web, Android, or another engine). Do this before scrolling: separating logical entities from nodes is also needed for virtualization.

1. Extract the **Model**: pure C++ without `cocos2d` (entities, stats, buffs, abilities, AI, spawners, waves, and level rules), its own vector and rectangle types, and fixed-step simulation with deterministic randomness.
2. **Ports** (interfaces): rendering, input, audio, storage, config source, and clock. Cocos implements them in the View layer.
3. **ViewModel**: HUD values, progress, and events (enemy destroyed, buff, wave). The View only displays them.
4. Replace `cocos2d::Value` in configs with a neutral JSON representation; replace the engine scheduler with a custom tick.
5. Replace singletons with dependency injection (`GameContext`).
6. Run tests and bots against the Model without a window, at maximum speed, with a random seed.

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

### Game objects off the engine node (plan, 2026-11-26)

Today `GameObject` holds a `cocos2d::Node*`: 161 `GetVisualNode()` uses (about 75 read the position, 8 the rotation, 9 the parent or scene), the object is destroyed from the node's exit callback, and `ObjectsFactory.cpp` mixes entity creation with about 400 lines of view code (texture, track layers, animation, z-order, color). Physics (`PhysicsObject`, `MovableObject`) uses `cocos2d::Rect` and `Vec2`. Order, each step green in build and tests:

- ⬜ 1. Model owns position and rotation: `GameObject` keeps `model::Vec2f Position` and `float Rotation` (`GetPosition()` / `SetPosition()`), the node only mirrors them. Replacing `GetVisualNode()->getPosition()` by `GetPosition()` is mechanical.
- ⬜ 2. Visual port `ports/IObjectVisual` (`SetPosition`, `SetRotation`, `SetVisible`, `PlayAnimation(id)`, `Release()`); `GameObject` holds it instead of `Node*`; `CocosObjectVisual` wraps the node.
- ⬜ 3. Physics on model types: `cocos2d::Rect/Vec2` become `model::RectF/Vec2f` in `PhysicsObject` and `MovableObject`.
- ⬜ 4. Split the factory: the model factory builds the entity from the config; a view-side factory (`view::cocos`) builds node, textures, tracks and animations and attaches the visual. The node stops owning the object's lifetime.

### Own base library (`Classes/templates/`, plan 2026-11-24)

Rules: code-style.md, "Own templates". Order:
1. ✅ (2026-11-24) Folder and renames: `TypeList` → `TTypeList`, `ManagerSet` → `TManagerSet` moved to `templates/`; rule for the `T` prefix.
2. ✅ (2026-11-25) Callbacks: `TFunction`/`TFunctionRef` everywhere, 0 `std::function` outside `tests/` and `templates/`. ⬜ `TDelegate`/`TMulticastDelegate` have no user yet.
3. ✅ (2026-11-25) `TStoredString`: `FPersistentString` / `FTransientString` (4-byte ids, `View()` is stable). Used by `ResourceTable`, `ConfigRefs`, `ConfigReader`, `ObjectsFactory`, `GameEvents`. Rule: read through `std::string_view`, store and compare ids as `FPersistentString`. ⬜ Remaining: ~1450 `std::string` uses (Cocos boundary, UI, loaders, tests); migrate per area when it is touched, loaders first.
4. 🔧 Containers: `TArray` (pmr), `TMap` (sorted flat map; with `FPersistentString` keys the order is intern order, not alphabetical), `TSmallArray`, `TEnumArray`, `TEnumFlags` done. ⬜ Remaining: ~505 `std::vector` and ~53 `std::map`/`std::unordered_map` uses; arenas Frame/Level/Session owned by the simulation.
5. ✅ (2026-11-25) `TSlotMap`, `TObjectPool`, `TRingBuffer`, `TSmallArray`, `TEnumArray`, `TEnumFlags`, `TScopeGuard`, `TObserved`, `THandle` done. Smart pointers: `shared_ptr` and `unique_ptr` are gone (`TSharedHandle`, `TUniquePtr`, 2026-11-26); `std::string` (~1450), `std::vector` (~505), `std::map` (~53) remain.
6. ✅ (2026-11-25) Allocation counter: `platform/AllocationCounter` (Debug/Test only) is logged by the crowd benchmark (`heap: N allocations, MB allocated, peak live, peak stack`). Baseline 2026-11-25 (40 s crowd): ~52000 allocations (~1300/s), 2.1 MB allocated, peak stack ~92 KB. Compare after each migration step.
   - `TSlotMap<T>` + `TEntityHandle` (index + generation): model entity ids for "the Cocos object is a view of a model entity", safe after delete.
   - `TObjectPool<T>`: node pools for row virtualization and bullets.
   - `TEnumArray<E, T>`: array indexed by an enum with the `static_assert` on its range (replaces enum value lists).
   - `TEnumFlags<E>`: type-safe bit flags (`ObjectEnums`, `AiSwitches`).
   - `TRingBuffer<T, N>`: event log, input history, replay buffer, without allocation.
   - `TSmallArray<T, N>`: inline storage that spills to the arena (hit lists, neighbor lists).
   - `TScopeGuard` and `TObserved<T>` (value with change event, for the ViewModel).
   - 🔧 Move `TRefCounted`/`THandle` to `templates/` (the forwarding header exists, but the definitions remain in `model/RefCounted.h`); move `TConfigCatalog`/`TWeightedTable` when touched.

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
- ⬜ Still on the engine: `GameScene` / editor / test-runner keyboard listeners, `ECommandType` (`ObjectEnums.h` includes cocos2d), config files read through `FileUtils` (`ConfigReader`).

## ⬜ Level editor (resume after the HUD, base, and scrolling)

The editor currently supports an 8-tile palette, mouse drawing, an enemy counter, saving, and testing. It only writes the simple level format (one wave, default spawner).

Plan:

1. **Spawners.** Add spawners `E`, `1`–`9` to the palette. Clicking a spawner opens a panel:
   - pool: enemy classes (Light / Medium / Heavy / Boss) and/or specific enemies from `EnemyCatalog`;
   - restrictions: which enemies and classes **not** to spawn;
   - a colored marker on the map and the spawner's number.
2. **Waves.** A separate panel with a wave list (add / remove / reorder); each wave has `delay`, `max_alive`, `interval`, and rows for “spawner × count.” Show the total enemy count and write the extended format (`spawners`, `waves`).
3. **Enemy builder.** Create `configs/enemies/<Id>.json` with class, stats (sliders), abilities (a list with parameters), buffs, AI, color, reward, and a tank preview. Save to a writable folder; `EnemyCatalog` picks it up without rebuilding.
4. **Buff and ability builders** (similar): edit `buffs.json`, stat modifiers, and icon.
5. **Level settings:** name, background / decor, base (see below), and level size (see scrolling).
6. **Conveniences:** undo/redo, fill and rectangle tools, mirror the map, multiple custom levels (slot list), import / export files, and reachability validation (check that the player / base is not walled in).
7. **Test launch** from any point; return to the editor with the same state.

## ⬜ Base defense

The base is optional and enabled per level (levels without a base continue to work as they do now).

- Add a `B` (base) map tile and a `Base : PhysicsObject` object (data in `configs/base.json`: health, sprite, ruined sprite).
- Add a `"base": { "health": 10 }` level field; without it, ignore the `B` tile or report a validation error.
- Conditions: destroying the base means defeat (like the player's death); victory requires all waves destroyed and the base alive.
- Enemies: a `BaseHunter` AI class that leads tanks to the base (grid pathfinding, navigating around walls, shooting through bricks); some enemies may choose between targeting the base or the player.
- HUD: show the base health bar in the top bar and warn when it is attacked.
- Bonuses: “base repair,” “fortification” (temporary walls around the base), and a “base shield” ability.
- Editor: base tile and a health field in level settings; validate that there is exactly one base and enemies can reach it.

## ⬜ Scrolling and large levels

Levels are not always a single screen: JSON defines the map size (any size), player start point, zones, and spawners across the map; the player can move in any direction.
- **Minimap** as a separate UI element: level outline, player position, base, spawners, and enemies within view range; can be opened as a full map.
- Level format: `size` (columns × rows), `player_start`, chunks or contiguous rows; remain compatible with current single-screen levels.

Goal: levels extend beyond the screen; all tanks and objects exist virtually, while only the visible area is rendered.

**Data model**
- The world is larger than the screen: `rows` can have any height (for example, 40–200 rows); width remains 16.
- Logical entities (`GameObject`) exist independently of nodes; create a node (`Node`) only for an entity inside the view window.
- Store static tiles (walls, bonuses) as map data (a grid), not as millions of objects.

**View window (virtualization)**
- The camera follows the player with smooth damping and respects level boundaries.
- The window contains the visible rows plus a **2-row margin above and below**. Prepare the next row as the camera moves so it appears smoothly, without jumps.
- Return nodes for rows leaving the window to a **pool** and reuse them for incoming rows (recycling), rather than creating them again.
- Preserve state: a damaged wall remains damaged when its row re-enters the window (state lives in the data; the node is only a view).

**Simulation**
- Enemies far from the camera use a simplified mode (grid movement and AI without nodes or animations); enemies near the window (within ≤ N rows) run fully.
- Move collision and lookup (`findObjectIf`) to a spatial grid (row buckets) to avoid scanning everything.
- Bullets exist only in the active area; remove them outside it.
- Spawners work across the whole world; waves can be tied to zones (trigger: the player reaches row N).

**Integration**
- HUD and level minimap (player, base, and enemy positions).
- Editor: map scrolling, level height changes, and wave zones.
- Save progress within a large level (checkpoints).
- Performance: test with 200 rows and hundreds of enemies; show a node counter in debug mode.

> Order: infrastructure → engine-independent core (MVVM) → base defense → scrolling → editor.

## ⬜ Next in queue

- Balance upgrade prices and level difficulty
- Map buff bonuses (shield and fire-rate tiles)
- Music and more sounds; volume settings
- Controls: gamepad and key rebinding
- Multiple players (local co-op)
- Mobile platforms (on-screen buttons and HUD scaling)
- Tests: automatically validate configs (JSON validator at startup)
