# Configs

All game data lives in `Resources/configs/` as JSON (rule: no hard-coded data in game code, see `.claude/rules/code-style.md`). Keys are `snake_case`. Level format: `docs/LEVEL_FORMAT.md`.

## How configs are found

- `ConfigReader::ReadObjectConfigs("configs/")` loads every file recursively at start. The id of a file is its path relative to `Resources/` (`configs/enemies/Basic.json`).
- `configs/resources/configs.json` maps short ids (`cfg.wall_brick`) to paths; code asks `ResourceManager::ResolvePath(id, ResourceKind::Config)` with a constant from `core/ResourceIds.h` (`ids::configs::*`). Other sections: `atlases.json`, `sounds.json`, `fonts.json`.
- Rule (done, 2026-11-23): code and JSON refer to a config by id, never by a raw path. Ids written inside JSON (`pickup`, weapon/ability `bullet`, `summon`, `split_into`, gallery `config`/`level`, palette/legend `config`) are resolved with `config_refs::ResolveName` (`services/ConfigRefs.h`). Moving a file only edits its registry line.
- Groups are registry prefixes, not folder scans: `cfg.enemy.*` (`EnemyCatalog`; the enemy id is the file name, so it is unique; `cfg.spawned.*` is not an enemy), `cfg.level.N` (campaign, listed in natural order by `config_refs::ListGroup`), `cfg.ui.*`, `cfg.strings.*`, `cfg.test.*`, `cfg.test_level.*`. A new enemy or level = a file + one registry line.
- Checks: unit tests (every registered file exists, every config file is registered, every `cfg.` string inside JSON resolves, every `ids::configs::All` id resolves) and `python tools/clang_check.py` (no quoted `"configs/` literal in `Classes/` outside `ResourceIds.h`, `GameConstants.h` and `tests/`).
- Bootstrap: `ConfigsFolder` and `ResourcesConfig` (`core/GameConstants.h`) are the only paths in code; `UiStyle` is created after `ResourceManager` and reads all configs once (`ReadObjectConfigs` skips ids already loaded).

## Target folders

An id is the path without `.json` (`objects/spawnables/walls/brick`).

| Folder | Holds | Read by |
|---|---|---|
| `resources/` | id -> file registry (atlases, sounds, fonts, configs) | `ResourceManager` |
| `levels/` | `campaign/level_N.json`, `test/` fields | `LevelLoader` |
| `objects/spawnables/` | things a level map or editor places: `walls/` (brick, stone, wood, base), `props/` (crate, fuel_barrel, level_mine, level_wreck) | `ObjectsFactory` |
| `objects/movables/` | `tanks/` (player, `enemies/`, `spawned/`), `projectiles/` (bullets, mine) | `ObjectsFactory`, `EnemyCatalog` |
| `objects/pickups/` | bonuses (`bonus_*`), the contents of crates and drops | `ObjectsFactory` |
| `balance/` | numbers: drops, difficulty, death_penalty, upgrades, buffs, side_hits, achievements, `ai/` | managers, `model` books |
| `views/` | looks and effects: effects, tank_look, tank_wreck, level_wreck, side-hit sparks, background | `Effects`, `TankLook`, view code |
| `ui/`, `input/`, `strings/`, `editor/` | layout, key bindings, texts, palette | `UiStyle`, `InputManager`, `StringsManager`, `EditorScene` |
| `audio/`, `platform/` | music lists, display and window defaults | `MusicManager`, `DisplaySettings` |
| `test/` | fixtures and galleries used by tests only | tests |

Rules:
- One file = one concern. An object file holds its schema (`object_type`, `node_type`, `texture_path`, `zorder`) and a `look` id; numbers go to `balance/`, frames, colours and effects to `views/`.
- A file that is not an enemy never sits under the enemy scan folder (`spawned/` is not spawnable).
- Documentation lives here and in `docs/`, not in README files next to the configs.

## Object types (`object_type`)

| Type | Class | Examples | Notable keys |
|---|---|---|---|
| `Wall` | `Wall` | brick, stone, wood, base, crate, fuel_barrel, level_wreck, tank_wreck | `cells`, `cell_health`, `destroyable`, `drop_table`, `blast_radius_tiles`, `hit_effect`, `contents` |
| `Bullet` | `Bullet` | bullet*, mine, level_mine | `speed`, `damage`, `team`, `bounces`, `lobbed`, `range_tiles`, `blast_radius_tiles`, `edge_effect`, `fade_effect` |
| `Bonus` | `Bonus` | bonus_* | `bonus_type` (Ammo, Repair, Buff, Coins), `amount`, `buff`, `lifetime`, `pickup_effect` |
| `Vehicle` | `Tank` | player, enemies | stats, `abilities`, `ai`, `drop_table`, `phases`, `split_*`, turn keys |

## Current state (2026-11-23)

- About 40 files sit flat in `configs/`; the target layout is not applied yet.
- Plan and order: `.claude` plan "Configs reorganisation": 1 docs, 2 legend into data, 3 id layer (done), 4 move files, 5 split mixed files, 6 shared base classes.
- Mixed files to split: `tank_look`, `side_hits`, `tank_wreck`, `level_wreck`, `music`, `upgrades`, `buffs`.
