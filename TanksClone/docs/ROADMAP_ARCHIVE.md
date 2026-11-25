# Roadmap archive

Finished work moved out of ROADMAP.md (2026-11-24). History, not needed at session start.

## Completed

- ✅ Refactoring: `tanks` namespace, `constexpr` instead of macros, `unique_ptr`, cross-platform `ConfigReader`.
- ✅ Menu, settings (language, audio), upgrade shop, pause, result screens, profile saves.
- ✅ Gameplay: bullets, walls (multiple destruction stages), bonuses, and grid-aligned collisions.
- ✅ Stats, buffs, and abilities as separate entities; enemies as JSON (`configs/enemies/`); `EEnemyType` is the enemy class.
- ✅ Spawners (`E`, `1`–`9`) with pools and restrictions, waves, and a separate boss spawner (level 6).
- ✅ Levels 1–6 and a simple-format editor (one wave).

## Current status (session handoff)

- Build: `MSBuild proj.win32\TanksClone.sln /p:Configuration=Debug /p:Platform=Win32 /p:PlatformToolset=v143 /p:WindowsTargetPlatformVersion=10.0.22621.0 /m`. Stop `TanksClone.exe` before building.
- Tests (`TanksClone.exe --test` in Debug and Test; report: `test_report.txt`; all pass): 43 unit tests, 12 level autotests (`PlayerBot`, a no-bot scenario, and an enemy crowd), and 32 menu autotest steps (including FPS measurement). The bot loses on levels 1–6 (expected; these tests check that enemies spawn) and wins on test fields. Tests write the profile and custom level to a temporary folder (`%TEMP%\TanksCloneTests`) and do not touch the real player profile.
- Bullet CCD and the FPS limit are verified; `--test` passes.
- File structure, builds, `dist`, and releases are documented in `README.md`.
- Only `proj.win32` works. `proj.android`, `proj.ios_mac`, and `proj.linux` are still templates (they reference `HelloWorldScene.cpp`, `MyGame`, and `org.cocos2dx.*`) and do not build.
- Next: real art and fonts (see “MVP cleanup”), then continue in the order below.

## ✅ MVP cleanup (November 2026)

- ✅ Removed Cocos template leftovers: `HelloWorld.png`, `CloseNormal/Selected.png`, the `minimalist` atlas (borrowed frames), empty `Resources/res/`, `TEXT_1..3` strings, template license headers and comments in `AppDelegate`/`main`, and names in `game.rc`.
- ✅ Replaced the Cocos app icon with a tank icon for Windows (`.ico`), Android, iOS, and macOS (`tools/gen_app_icons.py`).
- ✅ Engine statistics are available only in Debug (F3); `--test` is available only in Debug (`core/BuildInfo.h`).
- ✅ Resources are located beside the exe (`<exe>\Resources\`), not in the working directory.
- ✅ `Resources/` structure: `configs/`, `textures/` (`common`, `player`, `abilities`, `enemies/<Id>`, `tilesets/<name>`), `sounds/`, and `fonts/`. Source frames are in `art/`.
- ✅ Atlases: `tools/pack_atlases.py` builds `.plist` + `.png` from `art/` (no third-party packages; use `--check` to validate). Frame names are unique; tileset frames have prefixes (for example, `desert/wall_brick`).
- ✅ Resource manifest `configs/resources.json` + `ResourceManifest`: shared atlases load at startup; a level loads only its tileset and atlases for enemies that its waves can spawn (`AtlasScope`, reference counting), then unloads them on exit. Enemies specify their atlas in the `atlas` field; levels specify their tileset in `tileset`.
- ✅ `field` tilesets (levels 1–3) and `desert` (4–6): ground uses four tile variants instead of being drawn in code.
- ✅ Unique placeholder art (`tools/gen_placeholder_art.py`): a distinct untinted sprite for each enemy, plus the player, bullets, explosions, bonuses, spawn marker, ability and buff icons, and walls with three destruction stages.
- ✅ Sounds are synthesized by `tools/gen_placeholder_sounds.py`; new sounds are in use for level start, wall destruction, and boss explosion. Previous (likely borrowed) sounds were moved outside the project to `../reference_assets/sounds/`.
- ✅ Builds: post-build `tools/make_dist.ps1` creates `dist/Debug`, `dist/Test`, and `dist/Release` with only the required files (exe, engine DLLs, `Resources/`; Debug + `.pdb`, Release + VC runtime). Release archive: `tools/make_release.ps1`. Build outputs are in `.gitignore`.
- ✅ Resource tests: every frame referenced by configs exists in an atlas; a level loads only what it needs; an atlas stays loaded while any scene needs it; two tilesets do not interfere with each other.

**Release build:**
- ✅ 3D physics (Bullet) and Box2D are disabled in the engine: their Win32 libraries are absent from `cocos2d/external`, and the game does not use them. Engine changes: `cocos2d/cocos/2d/cocos2d_headers.props` (`CC_USE_3D_PHYSICS=0`) and `cocos2dx.props` (omit `libbox2d.lib` and `libbullet.lib`). Originals: `../reference_assets/*.props.orig`. 2D physics (Chipmunk) remains enabled because its library is available.
- ✅ Third configuration, `Test`: Release optimizations and engine, with `--test` and F3 enabled (`TANKS_DEBUG_TOOLS`, `HasDebugTools` in `core/BuildInfo.h`); `dist/Test` includes `.pdb`. The engine builds as Release for this configuration (mapped in `.sln`). `--test` takes about 29 s in Test versus 37 s in Debug. CMake does not have this configuration (CMake builds are unsupported).
- ✅ Release builds; `dist/Release` (~7 MB in the zip, including the VC runtime) runs from any working directory; `--test` is ignored in Release. Archive: `tools/make_release.ps1` → `dist/TanksClone-<version>-win32.zip`.

**Real art / licensing needed before public release:**
- ⬜ All graphics are generated 32×32 placeholders: sprites for 6 tanks (3 movement frames), bullets, 5 explosion frames, 4 bonuses, 5 ability icons, 2 tilesets (ground, brick, stone, wood), a spawn marker, and the app icon.
- ⬜ Sounds are simple synthesized placeholders (12 total).
- ✅ **Fonts:** replaced template `Marker Felt.ttf` (Apple) and `arial.ttf` (Microsoft) with OFL fonts that support Ukrainian Cyrillic: Press Start 2P (pixel-style headings) and PT Sans (UI); licenses are in `fonts/OFL-*.txt`. Old fonts moved outside Git to `../reference_assets/fonts/`.
- ✅ Ukrainian localization (`configs/strings/ukrainian.json`); settings languages: English → Ukrainian → Russian. Test verifies that each language contains every English string.
- ⬜ No base sprites or enemy previews for the editor yet (to be added with those features).

## ✅ Machine gun and cell-based wall destruction (November 2026)

- ✅ **Second player weapon — machine gun:** tank configs have a `weapons` list (`id`, `name`, `icon`, `bullet`, `reload_time`, `damage`, `uses_ammo`); switch with Q / Tab; each weapon reloads independently. The machine gun uses no ammunition, reloads in 0.12 s, deals 1 damage, and fires small bullets (`bullet_mg.json`); it cannot damage stone and slowly chips brick. Enemies without `weapons` keep one cannon as before.
- ✅ HUD: slot 0 shows the active weapon icon and Q; the ammunition row shows the machine-gun name and INF when it is active.
- ✅ The bot switches to the machine gun when out of shells and switches back when shells are available.
- ✅ **Walls made of subcells:** engine-independent `model/WallMask` stores a grid of cells with health. A hit removes cells from the side the bullet came from (width from the bullet's `chip_width`, depth from `chip_depth`, damage from `chip_damage`). An explosion can also hit an adjacent wall if the bullet struck between them. Tank collisions (`blocks`) and bullet CCD (`sweepHit`) use the remaining cells, so tanks can pass through a breached opening.
  - Brick: 4×4 cells with 2 HP each; a shell removes the full width for 2 rows (2 shells make a passage, as before). Wood: 2×2 cells with 1 HP each (1 shell). Stone: only bullets with `breaks_stone` can damage it (none yet).
  - Rendering: cells are child sprites using the same atlas frame; damaged cells are darkened. Old `*_damaged_1..3` frames were removed.
- ✅ Tests: unit tests (side-based chipping, cell depth and health, passability, cell-based CCD, explosion between two walls, stone; weapon switching, machine gun without shells, enemies with one cannon); level autotests (bot wins using only the machine gun; bot does not get stuck before a solid brick wall; a moving, firing tank breaches and passes through a wall); menu autotests (settings: language, audio, FPS; shop: insufficient coins and purchase; level: pause, resume, results screen, return to menu from pause; editor: saving a custom level).
- ⬜ Debris animation when cells are chipped, machine-gun sound, and a separate boss bullet that “breaks stone.”

## ✅ Fixed: overlapping tanks, turning jolt, and FPS limit (November 2026)

- ✅ **Tanks got stuck overlapping.** Causes: the spawner checked a spawn point only when the 0.5 s spawn animation began, so a later wave could use the same point; and a tank already overlapping another could not move at all. Now the point is reserved during the animation and checked again before spawning (if occupied, the enemy is queued again); movement is allowed when overlap does not increase (tanks can separate but cannot push deeper); overlapping tanks separate along the smaller axis, no faster than their movement speed and without entering a wall.
- ✅ **Turning jolt.** Grid alignment is no longer instant: the tank moves smoothly toward the tile center at its movement speed and stops if that point is occupied.
- ✅ **FPS limit did not work in-game.** The limit applied globally (including menus), and disabling vsync had no effect: the game called its own GLFW copy instead of the one in `libcocos2d.dll`. The menu now stays at 60 FPS; a level applies the profile limit on entry and restores 60 on exit. Windows vsync uses `wglSwapIntervalEXT`; the Windows timer is set to 1 ms (`timeBeginPeriod`) so `Sleep` in the main loop does not round a frame to 15.6 ms.
  - On a hybrid-graphics laptop (NVIDIA rendering, AMD 144 Hz display), an unlimited level runs at about 72 FPS with vsync off; this is a display-output-path limit, not a game limit.
- ✅ Tests: unit tests (overlapping tanks separate without entering walls; deeper overlap is blocked; turning has no jump; sideways correction is no faster than movement; spawners do not place enemies inside other enemies; level and menu frame intervals); autotests (40-second enemy crowd: no pair overlaps for more than 1 s; level FPS is about 30 with a 30 FPS limit, above 66 with a 120 FPS limit and with no limit, with vsync off; menu stays near 60 FPS).

## ✅ Movement without grid alignment (November 2026)

- ✅ Turning changes direction only; position does not change (instantly or gradually).
- ✅ Corner sliding: if an obstacle blocks a forward step, shift the tank sideways toward the opening when the required shift is within `corner_assist` pixels (JSON, one-third of a tile by default); otherwise, stop the tank.
- ✅ Enemy AI delays turning across its direction of travel until reaching the tile center (or hitting an obstacle, but no longer than 1 s).
- ✅ When no enemies are present, the bot moves back to its row instead of standing at its spawn point (an occupied point does not release another enemy).
- ✅ Tests: turning causes no displacement; a tank that clips a corridor corner enters it; a tank mostly blocked by a wall stops.
- ✅ Version history in `docs/history/` (screenshots, clip, `CHANGELOG.md`); `--showcase <folder>` mode (Debug and Test) captures the menu, settings, shop, editor, levels 1–6, boss, and a 5-second bot clip; `tools/make_gif.py` creates a GIF without external programs.

## ✅ Fixed from feedback

- Double explosion when a block is destroyed: now only the block explodes; a bullet explodes only for large bullets (`explosion_on_hit`) at the point of impact.
- Removed the buff bar from the HUD (the shield cooldown is already visible in its slot); the shield icon is drawn as a vector.
- Levels 2 and 4 had no enemies: fixed (touching a wall blocked the spawn point). Covered by `--test`.

## ✅ MOBA-style HUD (initial version)

- ✅ Top level bar: name, wave, enemy progress bar, score, coins.
- ✅ Bottom player panel: portrait, health, ammunition, reload, stats, ability slots (hotkey, icon, cooldown overlay with seconds), and buff icons with bars beneath them.
- ✅ Visual shield on the tank (bubble).
- ⬜ Animation: shield flickers before expiring; ability tooltip.
- ✅ Ability and buff icons use the project's own atlas (`textures/abilities`) instead of borrowed frames.

## 🔧 Current: infrastructure (before base defense and scrolling)

1. ✅ **File structure.** Organize `Classes/` into folders: `core/` (enums, constants, configs), `game/objects/` (tank, bullet, wall, etc.), `game/components/` (stats, buffs, abilities), `game/ai/`, `game/` (factory, spawner, levels), `services/` (audio, profile, upgrades, strings), `ui/` (widgets, HUD), and `scenes/`. Includes are relative to `Classes/`; `.vcxproj` and CMake are updated.
2. ✅ **Unit tests** without external dependencies (a minimal custom runner in `Classes/tests/`): stats and buffs, spawner pools and restrictions, level parsing and validation, waves, upgrades, and profile saves.
3. ✅ **Level autotests:** the player bot (`PlayerBot`: moves toward the nearest enemy, shoots, and uses abilities) completes each level in accelerated time without mouse or keyboard input. `TanksClone.exe --test` runs unit tests, then all levels, and reports to the log and exit code (for CI).
4. 🔧 **Resources and textures** (mostly done; see “MVP cleanup”):
   - ✅ Inventoried assets, removed template leftovers and borrowed frames, and used previously unused sounds (level start, wall destruction, boss explosion).
   - ✅ `Resources/` structure, script-generated atlases, and level resource manifest.
   - ⬜ **Resolution:** sprites are 32×32 and scaled ×2 (pixel art, `setAliasTexParameters`). Prepare 1×/2×/4× sets (sharp pixel scaling at integer ratios); check `SHOW_ALL` at different aspect ratios, large fonts, and the HUD at 4K.
   - ⬜ **Real art** to replace generated placeholders; base sprites (intact / ruined) and enemy previews for the editor.
   - 🔧 **Validation:** ✅ test that every config frame exists in an atlas; ✅ `pack_atlases.py --check` and unique frame-name checks; ⬜ find unused files and frames.
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
   - ✅ Step 7 done (2026-11-21): ports. `ports/` has `IAudioOutput` (play, stop all, preload with callback, unload), `IStorage` (named text files) and `IInputSource` (`model::KeyEvent`); Cocos adapters live in `platform/cocos/` (`CocosAudioOutput`, `CocosStorage`, `CocosKeyboardInput`). `GameContext` creates the adapters first (`PortAdapters` list) and hands them out through `GetAudioOutput()` / `GetStorage()` / `GetInputSource()`; a new platform replaces that list and the three getters. `AudioManager` and `SoundResource` play and preload through the audio port (`AudioManager::SetOutput` swaps in a fake for tests). The profile is now `profile.json` (`ProfileSerializer`, `JsonValue::ToString`) in the storage port, with a one-time conversion of the old `profile.plist` (`MigrateLegacyProfile`); the custom level goes through the storage port too. Key handling moved to engine-free `PlayerControls` (key events in, commands out), used by `KeyboardController`. Unit tests with fake audio and storage ports and fake key events. Loader flush (2026-11-21): `WorkQueue` has priorities (`WorkPriority::High/Normal/Low`), `Pump(budget)` (at least one job, then stops when the time is spent; `ResourceManager` uses `pump_budget_ms` from `resources.json`, 4 ms), `CancelAll()` (drops waiting, running and finished jobs without calling them) and `WaitIdle()`; `ResourceManager::CancelPendingLoads()` for a level change. Ready for streaming map chunks during gameplay; nothing streams yet. Still on the engine: `GameScene` / editor / test-runner keyboard listeners, `ECommandType` (`ObjectEnums.h` includes cocos2d), config files read through `FileUtils` (`ConfigReader`).
   - ✅ Placeholders (2026-11-21): fixed a normal launch showing no sprites (the id table latched empty when the loading screen asked for a font before the configs were read). `model::ResourceStatusBook` records every resource asked for (unchecked / ok / missing); `ResourceManager::FindFrame` gives a magenta checker frame for a missing frame, a missing font falls back to the system font, a missing sound is silent; each is logged once and not retried until the texture set changes (`ReloadAtlases`). Tests: startup order, placeholder frame, and a run-wide "nothing missing" check at the end of every `--test`. ⬜ Not yet: placeholder for missing configs; unused files and frames.
   - Order: id table + `AudioManager` on ids → configs (`GameConstants.h` paths, wall configs) → handles with ref counting → preload sets in `LoadingScene` → async → then the Audio/Input/Storage ports (MVVM step 7).

