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
| `objects/weapons/` | one file per weapon (`cfg.weapon.<id>`): bullet, reload, burst, icon, muzzle flash, sound. A tank lists ids: `"weapons": ["cannon", { "id": "shotgun", "locked": true }]`; the name key is `WEAPON_<ID>` unless the file has `name` | `TankLoader` |
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

## Where a key lives (split files, 2026-11-23)

| Subject | Schema / list | Balance (numbers) | Look |
|---|---|---|---|
| Walls and props (brick, stone, wood, base, crate, fuel_barrel, level_wreck, tank_wreck) | `objects/spawnables/...`: `object_type`, `node_type`, `texture_path`, `tileset_frame`, `zorder`, `scale`, `destroyable`, `cells`, `base`, plus `look` and `balance` ids | `balance/objects/<name>`: `cell_health`, `drop_table`, `blast_*`, `lifetime_seconds` | `views/objects/<name>`: `hit_effect`, `debris_*`, `edge_bleed_opacity`, `break_blast`, `color` |
| Tank look | - | `balance/tank_corpse`: `corpse_health`, `overkill_share` | `views/tank_look` (lamp, cracks, smoke, wreck colours and pieces); `views/side_hit_sparks`: `immobile_effect`, `immobile_interval` |
| Side hits | - | `balance/side_hits` | `views/side_hit_sparks` |
| Music | `audio/music`: track lists | `balance/music_rules`: fades, intense rules, volumes | - |
| Upgrades | `balance/upgrades`: id, cost, modifiers, requires | same | `views/upgrade_tree`: per upgrade `branch`, `step`, `icon` |
| Buffs | `balance/buffs`: id, duration, modifiers | same | `views/buff_icons`: per buff `icon`, `icon_color` |

Tank files: `move_animation` is `"vfx.<id>"` (like `sound_armor`: `"sfx.hit_armor"`); the definition (`frames`, `count`, `delay`) is that id in `views/effects`. Effect ids (`hit_effect`, `edge_effect`, `fade_effect`, `pickup_effect`) were already ids into `views/effects`.

`ObjectsFactory::ReadValues(id)` merges the `look` and `balance` parts into an object's values (keys in the object file win). A missing upgrade tree node or buff icon is logged.

## Widget files (`ui/widgets/<name>.json`, 2026-11-23)

One widget = one file, registered as `"cfg.widget.<name>"` in `resources/configs.json` (the prefix is a group: a new widget is a file plus a registry line). `WidgetFactory` / `LayoutWidget` (`ui/`) build it; `model::widget_layout::Arrange` (engine-free) places the children, so the layout re-flows whenever a text, an icon, the size or the text size changes.

| Key | Meaning |
|---|---|
| `layout` | `stack` (children centred, default), `row`, `column` (group centred, fixed `gap`), `anchor` (each child pinned to `anchor` [0..1, 0..1] of the inner area plus `offset`) |
| `size` | fixed width/height; 0 on an axis = as big as the content; `min_size`, `padding` [x, y] |
| `main_align`, `cross_align` | `start` (left/top), `center`, `end` |
| `scale_with_text` | sizes follow the player's text size (labels always do) |
| `on_click` | command id handed to the owner's command handler |
| `children` | list of parts, see below |

Child keys: `type` (`label`, `icon`, `rect`, `ring`, `spacer`, `widget` + `widget` id), `id` (the name the owner finds it by), `text` / `text_key`, `font_size`, `decorative`, `text_color`, `frame`, `size`, `anchor`, `offset`, `fill_parent` (takes the inner size, ignored when measuring), `fill` / `border` (a shared color name from `widgets.json` "colors", or `[r, g, b, a]`), `border_width`, `shrink` (a text wider than its room shrinks to fit), `wrap` (a label breaks into lines at the width its container gives it). Code can append nodes it made (buttons) to a widget (`AppendNode`), hide a child (`SetPartVisible`: no room, no gap) and replace the gap (`SetGap`). `ring` is a circular arrow (`size` = diameter, `border_width` = line, `fill` = color). Shipped: `button`, `keycap`, `icon`, `switch_bar`, `overlay` (+ `overlay_buttons`), `control_hints` (+ `hint`, `hint_pair`), `settings_panel` (+ `settings_buttons`), `main_menu` (+ `menu_buttons`).

## Adding a language (config only)

1. Copy `strings/english.json` to `strings/<name>.json` and translate the values (every key; a missing key falls back to the default language and is logged).
2. Add `"cfg.strings.<name>": "configs/strings/<name>.json"` to `resources/configs.json`.
3. Add `{ "id": "xx", "name": "<own name>", "strings": "cfg.strings.<name>" }` to `strings/languages.json` (`id` is what the profile stores; the Settings option cycles through this list in order). Optional `"fonts": { "decorative": "font.id", "plain": "font.id" }` (ids from `resources/fonts.json`) when the default fonts lack the letters.

Startup logs missing keys and characters no font can draw (`GlyphCoverage`); unit tests check both. An unknown saved language falls back to `default`.

## Plan

Steps: 1 docs, 2 legend into data, 3 id layer, 4 move files, 5 split mixed files (done), 6 shared base classes (open).