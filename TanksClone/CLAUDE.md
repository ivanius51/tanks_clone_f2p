TanksClone: 2D roguelike tank shooter in C++ (Cocos2d-x 3.17 for now; goal: an engine-independent core). The player clears map levels of any size (not always a single screen: a large map in JSON, a player start point, the camera follows the player in any direction), destroys waves of enemy tanks, collects coins and buys upgrades.

Architecture (goal): MVVM.
- Model: pure C++ with no engine types (entities, stats, buffs, abilities, AI, spawners and waves, rules, fixed-step simulation, determinism).
- ViewModel: state to display, plus events.
- View: a render adapter (Cocos now; later web, Android or another engine) connected through ports: render, input, audio, storage, configs.
Everything is OOP: encapsulated, with separate entities. Data lives in JSON: enemies, buffs, upgrades, levels, strings. A new enemy is a new file. The current code still mixes logic with Cocos; the migration plan is in ROADMAP.md.

Scope: a campaign of levels; waves with spawners (pools, limits); enemies built from combinations of stats, abilities and buffs; shop; save system; localization (uk/en); MOBA-style HUD; level and enemy editor; base defense (optional per level); large scrolling levels with virtualization (only the visible area plus a margin of rows is rendered); a separate minimap in the UI; texture atlases per level and per enemy; autotests with a bot.

Session start: before any work, read memory.md, all files in .claude/rules/ (code-style, architecture, workflow) and ROADMAP.md (plan and status). Don't rely on earlier conversation history.

Memory: save anything worth keeping between sessions (decisions, user preferences, findings, open problems) to memory.md in the repository root, not to the session's built-in memory. One short entry per fact, with the date (YYYY-MM-DD). Update or delete outdated entries instead of adding duplicates. Keep memory.md short: it is read at the start of every session.