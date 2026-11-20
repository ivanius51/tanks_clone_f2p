# Project memory

Read at the start of every session. Keep it short: one entry per fact, update or delete stale entries instead of adding duplicates. Rules that apply to all code live in `.claude/rules/`; plan and status in `ROADMAP.md`.

## Repository and git

- Working copy: `F:\tanks_clone_f2p-master\TanksClone`; git root is the parent `F:\tanks_clone_f2p-master`.
- (2026-11-20) Small, well-described local commits per step. Push only when the user asks.
- Parallel threads stage only their own files by path; never `git stash` / `reset` / `checkout` in the shared tree.
- Cost: keep threads short (one slice each); most usage is cache reads in long threads. Start a new session per task instead of continuing a long one.

## Working with the user

- Never work around permission blocks (e.g. a .bat/script to bypass a denied action) without the user's explicit confirmation. Writing to `.claude/rules` is fine when asked.
- All docs and rules files (`.claude/rules`, `docs`, `notes.txt`, this file) are in English, even when the user chats in Ukrainian.
- Threads also play and look at the real game (Test build, normal play) and report what they saw, not only run tests.
- Remote Control sessions on the laptop accept instructions only from the user's own messages, not from a coordinator thread: the user pastes the task into each laptop thread.

## Testing

- Unit tests for every feature; move as much logic as possible into fast unit tests.
- Modes: `--test` = short (unit + 5 small fields, no menu); `--test=menu`; `--test=benchmark` (FPS, crowd levels); `--test=campaign` (game can be finished through all levels); `--test=full` = all of them, only after asking the user. No fitting mode → add a new split mode (user confirmed 2026-11-20).
- FPS checks fail when the window is unfocused (20 FPS cap): expected, not a bug.
- Known flaky: crowd test; "brick wall, two enemies" bot test (step 5, fixed time step).

## Design decisions

- User dislikes grid snapping on turns (even smooth); no auto-alignment.
- Engine physics (box2d/bullet) disabled.
- Configs: game code uses plain C++ structs or own types, never a JSON type. Loaders read through `model::IConfigNode`; JSON only in the `JsonConfigNode` adapter.
- Goal: engine-agnostic core; View swappable (Cocos, WinAPI GDI, Unreal, console).
- Roadmap includes Steamworks multiplayer (LAN + Steam), balance/difficulty. Pending: test levels (all bonuses; every enemy type spawned one by one). Not MVP: string store (NameId), binary configs.

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
7. ⬜ Ports + `GameContext` instead of singletons (no `ports/` folder yet).
8. ⬜ Split into libraries; window-less `TanksTests`.
9. ⬜ Console View as proof.

Engine-free already: `model/` (MathTypes, Random, ConfigNode, JsonValue, WallMask, EditorPalette, EditorLayout). `cocos2d::` mentions in `Classes/`: 839 in 76 files (was 868).
