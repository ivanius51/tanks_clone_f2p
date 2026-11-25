# Resource and Content Rework Plan

Status: Phase 1 (plan), updated with user answers. See also `ART_GUIDE.md`. Date: 2026-11-20.

Phases, in order:
1. **Plan** (this doc).
2. **Content**: create assets, atlases and many config files (weapons, skills, buffs, enemies, player styles, blocks, backgrounds).
3. **Code**: only the loaders/systems needed so that phase 2 content works with no further code.

Guiding rule: **a new background, block, weapon, bullet, buff, skill, enemy, player style or explosion = new JSON file(s) + assets. No C++ changes.** Code reads configs only through `model::IConfigNode` loaders into plain structs.

---

## 1. Asset inventory

### 1.1 `reference_assets/Images` (source art, higher quality)

| Group | Files | Notes / use |
|---|---|---|
| Explosions (`sprites/explosions`) | `explosion_1..5.png`, `explosion.png`, `Explosion03.png` (+ duplicate `Explosion03 (2).png`), `Explosion21.png`, `exp.png`, `exp2.png`, `exp2_0.png`, `explode.png`, `explode_1.png`, `explong.png`, `expright.png`, `explosprite.png`, `nuvormexplosions.png`, `spritesheet3.png`, `boom32wh12.png`, `Effect47.png`, `Effect95.png`, `Aura38.png` | Multi-frame sheets. Use for: tank death (small/large/boss), bullet hit, wall break, spawn, shield/aura (`Aura38`), ability FX (`Effect47`, `Effect95`). Frame grid must be measured per sheet. |
| Bullets (`sprites/bullets`) | `bullet1..11.png` | 11 projectile looks: cannon shell, MG, plasma, rocket, etc. `bullet2..4` are big (~15 KB), likely animated or HD. |
| Tanks | `tankes.png`, `tanks.png`, `redtank.png`, `tank_green/pink/white/yellow.bmp` | Player colour styles (4 colours, multiplayer) and enemy bodies. |
| NES reference | `NES - Battle City JPN - General Sprites.bmp`, `... Miscellaneous.bmp`, `nesBattle.png` | **Reference only; do not ship** (copyrighted original game art). Use for layout/size intent. |
| Tiles | `tiles.png`, `tiles (1).png` (duplicated in `original/`) | Grounds and blocks: brick, stone, wood, water, ice, grass/bush. |
| Pickups | `coin.png`/`coin.bmp`, `diamond.png`, `diamonds32x5.png`, `diamonds32x24x5.png`, `gem.png`, `carrot.png`, `treasure.bmp`, `if_treasure_44827.png`, `block.png`, `htc2_0407.png` | Coins (animated 5 frames), gems as rarer currency, treasure chest drop. |
| Misc sheets | `objects.bmp`, `objects8.bmp`, `other.bmp`, `minimalist.png/.plist/.webp` | `minimalist.plist` is an existing atlas, likely UI icons. |
| Sounds | 12 wav (`base_explode`, `bonus_destroy`, `bonus_points`, `count`, `high_score`, ... ) | Several not in game: `base_explode`, `bonus_points`, `count`, `high_score`, `bonus_destroy`. |
| Junk | `sprites/explong.bmp` (3.2 MB, same as `explong.png`), `Explosion03 (2).png`, root copies of `tiles*.png`, `tank*.png` | Drop on import. |

Licensing (decided): **ship only self-made or self-generated art.** All of `reference_assets/` is reference only (timing, sizes, mood); it is never copied into `Resources/`. Art rules and budgets: `ART_GUIDE.md`.

### 1.2 Current `TanksClone/Resources`

- `textures/` (SD) and `textures_hd/` (HD), each: `common`, `player`, `abilities` atlases, `enemies/<Name>.plist` (Basic, Fast, Heavy, Sniper, Boss), `tilesets/field|desert.plist`.
- Atlases are tiny (SD `common.png` 1.4 KB, HD 34 KB): these are the generated placeholder art. **None of the reference art is in the game.**
- Explosion = `boom<N>` frames in `common` (`Effects.cpp`); spawn = `spawn_marker`.
- Sounds: 13 wav, generated.

### 1.3 Gap summary

| Need | Have in game | Source in reference |
|---|---|---|
| Animated explosions (several sizes) | 1 placeholder `boom` | 20+ sheets |
| Hit/impact, muzzle flash, spawn, shield aura | spawn marker only | `Effect47/95`, `Aura38`, small explosion sheets |
| Bullet variety | 2 (`bullet`, `bullet_mg`) | 11 |
| Player styles | 1 (green) | green/pink/white/yellow/red |
| Blocks | brick, stone, wood | + water, ice, bush, steel variants from `tiles*.png` |
| Backgrounds | field, desert | more ground sets cut from `tiles*.png` (snow, stone floor, dirt) |
| Pickups | coin-less placeholder icons | coin anim, gems, treasure |

---

## 2. Resource pipeline (target)

### 2.1 Folder layout
```
reference_assets/           # untouched source, never loaded by the game
art_src/                    # cleaned, sliced, named frames (PNG, HD size), committed
  explosions/<fx_id>/0001.png ...
  bullets/<bullet_id>/...
  tanks/<style_id>/body_0001.png ...
  tiles/<tileset_id>/<block_or_ground>_0001.png
  pickups/, ui/, fx/
Resources/
  textures_hd/  textures/   # generated atlases (HD = 2x SD), same plist frame names
  configs/...
```

### 2.2 Atlas groups (one plist + png per group, HD and SD)
| Atlas | Content | Loaded |
|---|---|---|
| `common` | UI, pickups, generic FX (small hit, spawn) | always |
| `fx/<fx_id>` or `fx_pack_<n>` | big explosion sheets | when referenced by loaded content |
| `players/<style_id>` | one player style | when that style is chosen |
| `enemies/<enemy_id>` | one enemy | per level, from its spawn rules (existing `AtlasScope`) |
| `bullets` | all projectiles (small) | always |
| `tilesets/<tileset_id>` | ground variants + blocks for that theme | per level |

### 2.3 Tooling (phase 2, scripts only)
- `tools/slice_sheet.py`: cut a sheet into frames from a JSON slice spec (`cols`, `rows`, `frame_w/h`, `count`, `margin`). Slice specs live in `art_src/slices/*.json` so re-slicing is reproducible.
- `tools/build_atlases.py`: pack `art_src/**` into HD atlases and SD (50%) copies, emit Cocos plist. Generalise the existing atlas generator.
- `tools/validate_content.py`: every id referenced in configs exists (frames, sounds, buffs, bullets, atlases). Also run as an autotest (`--test`).

### 2.4 Naming
Frame names: `<group>_<id>_<anim>_<NN>` e.g. `fx_explosion_big_07`, `tank_player_red_move_02`, `block_brick_damaged_1`. Configs reference animations by id, never by frame pattern strings.

---

## 3. Existing config schemas and what each lacks

| Schema | Where | Has | Missing for config-only variety |
|---|---|---|---|
| Vehicle (player/enemy) | `player_green_tank.json`, `enemies/*.json` | stats, weapons list (player), abilities, `move_animation` frame pattern, `atlas`, `ai`, score/coins | `style`/skin ref; weapons on enemies; death FX + sound ids; drop table; `buffs` on spawn (passive); `tags`; inheritance (`extends`) to avoid copy-paste |
| Bullet | `bullet.json`, `bullet_mg.json` | speed, damage, chip width/depth/damage, `explosion_on_hit` | hit FX id, trail FX, sound, `pierce`, `bounce`, `splash_radius`, `lifetime/range`, `homing`, applies-buff-on-hit, animation |
| Weapon | inline in player config | bullet path, reload, damage, uses_ammo, icon | own file `weapons/*.json`; `spread`, `pellets`, `burst`, `muzzle_fx`, `shot_sound`, `charge_time`, ammo per shot |
| Ability | inline `{type: Shoot/Buff, buff, cooldown}` | 2 hard-coded types | own file `abilities/*.json`; generic effect list (apply buff, spawn object, dash, heal, area damage, place mine); targeting (self / area / allies); icon, FX, sound |
| Buff | `buffs.json` (one file) | duration, stat modifiers (add/mul), `invulnerable`, icon | one file per buff; stacking rule; periodic effects (DoT/heal-over-time); visual FX (aura); flags as list (`invulnerable`, `stealth`, `ricochet`); new stats (see below) |
| Stats | `StatType` enum in C++ | MaxHealth, Damage, Armor, Speed, ReloadTime, MaxAmmo | stats are a **code enum**: adding a stat needs code. Add once: BulletSpeed, Crit chance, Pierce, Range, Regen, PickupRadius, CoinBonus, CooldownReduction. Long term: data-defined stat ids. |
| Bonus/pickup | `bonus_*.json` | type Ammo/Repair/Buff, texture | generic effect list (same as ability effects), coin/gem value, lifetime, blink, pickup FX/sound, drop weight |
| Upgrade | `upgrades.json` | stat modifiers per level, cost | unlocks (new weapon/ability/style), per-weapon upgrades, requirements |
| Wall/block | `wall_*.json` | destroyable, cells, cell_health | `blocks_bullets`, `blocks_tanks`, `slow` (sand/ice/water), `hides` (bush), damaged-stage frames, break FX/sound, debris, legend char |
| Tileset/background | `resources.json` tilesets + `background.json` weights | weighted ground variants `ground_N` | per-tileset file: ground variants + weights, decals, which blocks it reskins, music, ambient colour; adding a tileset currently needs a `resources.json` edit (ok) but weights are global |
| Level | `levels/*.json` | rows, tileset, spawners rules, waves | level **legend** (char → block/spawner/bonus id) instead of hard-coded chars; player spawn per style; bonus spawn table; per-level atlas list is derived |
| AI | `ai/tank_ai.json` profiles | tuning numbers | profiles are tied to the `EAiType` enum (Wander/Hunter); new behaviours need code. Add profile "behaviour" = composition of existing parameters |
| Enemy class | `EEnemyType` enum | Light/Medium/Heavy/Boss | fine as code enum, or move to tags |
| Effects | `Effects.cpp` hard-codes `boom%d`, `spawn_marker` | nothing in config | **no FX config at all**: needs `fx/*.json` |
| Sounds | file names in code | nothing | `sounds.json` id → file(s), volume, random variants |
| Player style | none | single `player_tank` texture | `styles/*.json` |

---

## 4. New config schemas (phase 2 creates content, phase 3 adds loaders)

File layout (decided): **grouped files with a base plus a folder of extensions.**
```
configs/Buffs.json            # base buff types (abstract templates, e.g. _stat_buff, _dot, _aura)
configs/Buffs/PlayerBuffs.json
configs/Buffs/EnemyBuffs.json
configs/Buffs/BossBuffs.json
configs/Buffs/SpecialBuffs.json
```
The same pattern applies to Weapons, Bullets, Abilities, Enemies, Blocks, Tilesets, Styles, Fx, Drops and Sounds. Rules:
- Every file is `{ "<id>": { ... }, ... }`; ids are global per kind and must be unique (duplicate = load error, caught by tests).
- `"extends": "<id>"` inherits from any entry of the same kind (base file or another group file); child keys override, objects merge deeply, arrays replace.
- Ids starting with `_` are abstract templates: inherit-only, never spawned or listed.
- All files in `<Kind>/` are loaded automatically, so a new group file needs no registration.
- Strings via string ids (`NAME_*`). The examples below show single entries for brevity.

```jsonc
// configs/fx/explosion_big.json
{ "atlas": "fx/explosions_a", "frames": "fx_explosion_big_%02d", "count": 16, "fps": 24,
  "scale": 1.0, "loop": false, "blend": "additive", "sound": "explode_big", "shake": 0.3 }

// configs/bullets/plasma.json
{ "visual": "bullet_plasma", "speed": 600, "damage": 2, "range": 900,
  "pierce": 0, "bounce": 1, "splash_radius": 0, "chip": { "width": 32, "depth": 1, "damage": 1 },
  "hit_fx": "hit_plasma", "trail_fx": "trail_blue", "on_hit": [ { "apply_buff": "burn" } ] }

// configs/weapons/shotgun.json
{ "name": "WEAPON_SHOTGUN", "icon": "icon_shotgun", "bullet": "pellet",
  "reload_time": 0.9, "pellets": 5, "spread_deg": 25, "ammo_per_shot": 1,
  "muzzle_fx": "muzzle_small", "sound": "shot_shotgun" }

// configs/abilities/dash.json
{ "name": "ABILITY_DASH", "icon": "icon_dash", "cooldown": 6,
  "effects": [ { "type": "apply_buff", "buff": "dash", "target": "self" } ], "fx": "dash_trail" }

// configs/buffs/burn.json
{ "name": "BUFF_BURN", "duration": 3, "stacking": "refresh", "icon": "icon_burn",
  "modifiers": [ { "stat": "Speed", "mul": 0.9 } ],
  "periodic": { "interval": 0.5, "effects": [ { "type": "damage", "amount": 1 } ] },
  "flags": [], "fx": "aura_fire" }

// configs/blocks/water.json
{ "char": "~", "visual": "block_water", "destroyable": false, "blocks_tanks": true,
  "blocks_bullets": false, "cells": 1, "anim": "block_water_anim" }

// configs/tilesets/snow.json
{ "atlas": "tilesets/snow", "ground": [ { "frame": "ground_0", "weight": 55 }, ... ],
  "block_skins": { "brick": "block_brick_snow" }, "music": "theme_snow" }

// configs/styles/player_red.json   (player skin, multiplayer colour)
{ "name": "STYLE_RED", "atlas": "players/red", "body": "tank_player_red",
  "move_anim": "tank_player_red_move", "tint": [255,80,80], "death_fx": "explosion_mid",
  "unlock": { "cost": 300 } }

// configs/enemies/flamer.json
{ "extends": "_enemy_base", "name": "ENEMY_FLAMER", "class": "Medium", "ai": "Hunter",
  "style": "enemy_flamer", "stats": { "health": 6, "speed": 90, "armor": 1 },
  "weapons": ["flamethrower"], "abilities": ["dash"], "passive_buffs": ["fire_resist"],
  "death_fx": "explosion_mid", "drops": "drops_medium", "score": 200, "coins": 20 }

// configs/drops/drops_medium.json
{ "rolls": 1, "items": [ { "bonus": "coin", "weight": 70 }, { "bonus": "bonus_repair", "weight": 10 }, { "none": true, "weight": 20 } ] }

// configs/sounds.json
{ "explode_big": { "files": ["sounds/explode_big_1.wav", "sounds/explode_big_2.wav"], "volume": 0.9 } }
```

Level legend (replaces hard-coded chars, default legend file shared by levels):
```jsonc
"legend": { "#": "brick", "S": "stone", "W": "wood", "~": "water", "%": "bush", "_": "ice",
            "P": { "player_spawn": 0 }, "E": { "spawner": "E" } }
```

Generic **effect list** (one small vocabulary shared by abilities, bonuses, bullet on-hit, periodic buffs): `apply_buff`, `remove_buffs`, `damage`, `heal`, `add_ammo`, `add_coins`, `spawn_object`, `spawn_fx`, `area` (radius + nested effects), `dash`. New content combines these; only a new verb needs code.

---

## 5. Content backlog for phase 2 (first batch)

- **FX (8)**: explosion_small / mid / big / boss, hit_spark, wall_debris, spawn_portal, shield_aura, muzzle_small.
- **Bullets (8)**: shell, mg, heavy_shell, plasma, rocket (splash), ricochet (bounce), pierce_rail, flame.
- **Weapons (8)**: cannon, machine_gun, heavy_cannon, shotgun, rocket_launcher, plasma_gun, railgun, flamethrower.
- **Abilities (8)**: shield, overdrive (rapid fire), dash, repair, mine_drop, emp_pulse (area slow), smoke (stealth), airstrike.
- **Buffs (12)**: shield, rapid_fire, speed_boost, armor_up, burn, slow, stun, regen, damage_up, stealth, ricochet_shots, magnet.
- **Enemies (12)**: existing 5 + scout, flamer, rocketeer, shielder (gives allies shield), healer, kamikaze, mine_layer, twin_boss.
- **Player styles (6)**: green, yellow, red, blue, white, pink (multiplayer colours).
- **Blocks (8)**: brick, stone, wood, steel, water, ice, bush, sand.
- **Tilesets/backgrounds (5)**: field, desert, snow, stone_floor, ruins.
- **Bonuses/drops**: coin, gem, chest, repair, ammo, each buff pickup; drop tables per enemy class.
- Test levels: one level per block type, a level that spawns every enemy one at a time, a bonus showcase level (extends existing `levels/test`).

---

## 6. Phase 3 code work (only what content needs)

1. Loaders (IConfigNode → struct) for: fx, sounds, bullets, weapons, abilities, buffs (per-file), blocks, tilesets, styles, drops, legend; `extends` merge in the config reader.
2. Registries by id (`ContentDb`) loaded at start, validated (missing ids = test failure).
3. Effect-list executor (the verbs above) used by abilities, bonuses, bullets, buffs.
4. Stats: add the new stat ids; plan move to data-defined stats.
5. FX player: replace hard-coded `boom%d` in `Effects.cpp` with `fx` ids.
6. Block behaviours: flags (blocks_tanks/bullets, slow, hides) instead of a wall subclass per type.
7. Atlas scope driven by referenced ids (style, enemies, tileset, fx).
8. Unit tests + `--test` autotests for every loader and a content validation test.

Coordinate with the ongoing IConfigNode refactor thread before step 1.

---

## 7. Decisions

Answered 2026-11-20:
1. Ship only self-made or self-generated art; reference folder stays as reference.
2. SD = pixel art (stylish, faster); HD = "light realism" in the spirit of the original, for big screens. Same content ids and gameplay sizes in both. Details: `ART_GUIDE.md`.
3. Grouped files: base `<Kind>.json` + `<Kind>/<Group>.json` extending it (see section 4).
4. Keep the reference folder. Placeholder atlases stay until each id has final SD and HD art made by the `ART_GUIDE.md` rules.
