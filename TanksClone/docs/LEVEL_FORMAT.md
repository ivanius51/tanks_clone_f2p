# Level format and editor

A level is one JSON file. Built-in levels are `configs/levels/campaign/level_N.json`, registered as `cfg.level.N` and played in the order of `configs/levels/campaign.json` (`cfg.campaign`: `{ "levels": ["cfg.level.1", ...] }`, read by `model::CampaignList`); the editor saves the player's level as `custom_level.json` through the storage port (`ports/IStorage`). Code: `game/LevelLoader.*`, `game/LevelDataLoader.cpp`.

## Keys

| Key | Required | Meaning |
|---|---|---|
| `name` | yes | String id (`LEVEL_1`) or plain text |
| `tileset` | no | Atlas key from `configs/resources/atlases.json` (`field`, `desert`) |
| `music` | no | Track id from `configs/music.json` / `resources/sounds.json` |
| `rows` | yes | 11 strings of 16 characters (`GridRows` x `GridColumns`), one character per tile |
| `enemies` | one of | Simple form: `{ "total", "max_alive", "spawn_interval", "types": [enemy ids] }` |
| `spawners` + `waves` | one of | Advanced form. `spawners`: one character -> `{ types, enemies, exclude_types, exclude }`. `waves`: `[ { delay, max_alive, interval, spawns: [ { spawner, count } ] } ]` |
| `ai_features` | no | `{ feature: bool }` switches (`dodge`, `lead_shots`, ...), applied over the AI profile, enemy config and difficulty |
| `script` | no | Scripted scene (`model::Script`): `actors` (`"a": "enemy#1"`) and `steps` (`wait`, `move_to`, `move_by`, `face`, `line_up`, `shoot`, `caption`, `until`, and setup steps `set_health` (share), `buff` (id), `tactic` (name), `end_level` (`win` or `lose`); `until` also takes `kills>=N`, `defeated>=N`, `pickups>=N`, `wave>=N` and `player.in_area:x,y,w,h`) |
| `fog_of_war` | no | `{ "enabled": false, "vision_tiles": 7, "show_initial_walls": true }`. Read, kept and written; nothing draws it yet (plan step 4) |
| any other key | no | Kept as is (`LevelData::Extra`) and written back, so an older editor never destroys newer data. Only top-level keys are kept |

## Saving

`LevelLoader::ToJson` calls `LevelDataLoader::WriteLevel`, which writes every `LevelData` field (name, music, tileset, fog_of_war, rows, enemies, spawners, waves, ai_features, script, then unknown keys) through the engine-free `model::LevelJsonWriter`. Layout is stable so git diffs stay small: tabs, keys in the order above (nested keys sorted), one map row per line, short objects and arrays on one line. `waves` is written only when the level has its own (a level whose single wave is the one made from `enemies` stays in the simple form). `LevelLoader::HasOwnPools` tells the editor not to replace `EnemyIds` of a level with its own spawners or waves. Limits: a float is written in its shortest form and `1.0` in an unknown key may come back as `1`; an empty array in an unknown key comes back as `{}`. The shipped level files are in this layout (a unit test checks that each equals what the editor would save), so a first editor save changes nothing but the edit. Nested keys follow a fixed order (`total`, `delay`, `max_alive`, `interval`, ..., `spawner`, `count`, ...), then sorted; an object or array that fits in 110 characters stays on one line. Numbers are never written in exponent form.

## Legend (characters in `rows`)

One table, `configs/level_legend.json` (`cfg.level_legend`), read by `model::LevelLegend` (engine-free, `model/LevelLegend.*`) and held by `LevelLegendManager`. `LevelLoader::Validate`, `LevelLoader::BuildPathGrid`, the `GameScene` spawn code and the editor palette all read it, so a new object is a data change only. Entry keys:

| Key | Meaning |
|---|---|
| `tile` | One character, or several (`"123456789"`) sharing the entry |
| `role` | `empty`, `wall` (spawned from `config`, tileset frame applied), `prop` (any object from `config`), `pickup` (`config` lies in a supply crate), `crate` (crate with a random pickup), `player`, `spawner` |
| `config` | Object config id (`cfg.*`); required for `wall`, `prop`, `pickup`; gives the editor picture and scale |
| `frame`, `scale` | Editor picture for entries without a config (player, spawners) |
| `pass` | Path grid: `open` (default), `breakable` (shot apart), `blocked` |
| `unique` | At most one on a map (`H`); the player start is always exactly one |
| `is_base` | Destroying it loses the level |
| `default` | The default spawner used by the simple `enemies` form |

Current characters (see the file for the truth):

| Char | Role | Object config | Pass |
|---|---|---|---|
| `.` | empty (also the eraser) | none | open |
| `#` / `W` | wall: brick / wood | `wall_brick` / `wall_wood` | breakable |
| `S` | wall: stone | `wall_stone` | blocked |
| `H` | base wall (unique, `is_base`) | `wall_base` | breakable |
| `X` | tank wreck | `level_wreck` | breakable |
| `P` | player start | player tank | open |
| `E` | default enemy spawner | marker | open |
| `1`-`9` | numbered spawners with their own pools (hand-written, not in the palette) | marker | open |
| `M` | mine | `level_mine` | open |
| `F` | fuel barrel | `fuel_barrel` | breakable |
| `A` / `R` | ammo / repair pickup | `bonus_ammo` / `bonus_repair` | open |
| `C` | supply crate | `crate` | breakable |

An unknown character is rejected (`LEVEL_ERROR_TILE`). The code keeps only `namespace tiles` (`Empty`, `Brick`, `Stone`, `Wood`, `PlayerSpawn`, `EnemySpawn`, `Ammo`, `Repair`) for maps it builds itself (editor start map, tests); a unit test checks they exist in the legend.

## Custom library

`model::CustomLibrary` (engine-free) keeps what the player made, through `ports/IStorage`: the index `custom_library.json` (`levels`: ids, `campaigns`: `id`, `name`, `levels`, `completed`, `coins`) and one `level_<id>.json` per custom level (same format as above). Ids are lowercase letters, digits and `_`. Removing a level takes it out of every campaign; changing a campaign's level list resets its progress. The older single slot `custom_level.json` still exists for the current editor.

## Editor

- Scene: `scenes/EditorScene.*`. The palette comes from `configs/editor/palette.json` (`tabs`: walls, units, bonuses, tools; `entries`: `tab`, `tile`, `name`, optional `icon: erase`). Picture and scale come from the legend entry of the tile. Parsed by `model/EditorPalette`; a tile must be one character present in the legend and its tab must exist.
- Layout and colours: `configs/ui/editor.json` (`layout`, `limits`, `colors`) via `viewmodel/EditorStyle`.
- Saving: `LevelLoader::ToJson` then storage as `custom_level.json`; the game loads it as the config id `custom_level`.

## Rules for a valid level

- Exactly 11 rows of 16 characters, only legend characters.
- One `P`, at least one spawner, at most one `H`.
- Every enemy id in `types` exists in the enemy catalog.
- The player must have a free path (`LEVEL_ERROR_BLOCKED` otherwise).
