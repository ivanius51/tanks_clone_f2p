# Art Guide: budgets and rules for SD (pixel art) and HD

Status: draft for review, 2026-11-20. Companion to `RESOURCE_REWORK_PLAN.md`.

## 0. Principles

- **Ship only self-made or self-generated art.** Use `reference_assets/` only for reference, such as sizes, timing and mood. Never copy from it or trace over it.
- **Two visual modes, one set of content ids.** Every visual id, such as `tank_player_red` or `fx_explosion_big`, exists in both modes, and configs never mention the mode.
  - **SD = pixel art.** It is crisp, stylish and cheap to render, and it is the default for low-end devices.
  - **HD = "light realism".** It has painted or rendered shading but keeps the Battle-City silhouettes and readability, and it is meant for big screens.
- **Gameplay sizes are equal in both modes.** Collision is in world units. Only the texture resolution and the art style differ. A world tile is 1 unit, which is 16 px in SD art and 64 px in HD art.
- **Readability first.** Team colour, the facing direction and a bullet against any ground must be readable at 100% zoom on a phone.

## 1. Grid and sizes

| Item | World tiles | SD px (pixel art, drawn 1x, shown at integer scale) | HD px |
|---|---|---|---|
| Ground tile | 1x1 | 16x16 | 64x64 |
| Block (4 sub-cells for chipping) | 1x1 | 16x16 (cells 8x8) | 64x64 (cells 32x32) |
| Tank (normal) | 1x1 (sprite may be up to 1.25 for barrel) | 16x16, max 20x20 | 64x64, max 80x80 |
| Boss tank | 2x2 | 32x32 | 128x128 |
| Bullet | ~0.25 | 4x4 to 6x8 | 16x16 to 24x32 |
| Pickup | 1x1 | 16x16 | 64x64 |
| FX small (hit, muzzle) | 0.5 to 1 | 8 to 16 | 32 to 64 |
| FX mid (tank death) | 2 | 32x32 | 128x128 |
| FX big (boss, rocket splash) | 3 to 4 | 48 to 64 | 192 to 256 |
| UI icon (ability, buff) | n/a | 16x16 | 64x64 |
| HUD portrait / style preview | n/a | 32x32 | 128x128 |

- SD is rendered with nearest-neighbour filtering at integer scale (x2, x3, x4) and never with sub-pixel scaling. The camera snaps to whole screen pixels.
- HD uses linear filtering and mipmaps off for sprites. Sizes are powers of two or multiples of 16.
- Sprites face **up** (north) in source, and the game rotates them. Light comes from the **top-left** in both modes.

## 2. Palettes

**SD**
- One master palette of **32 colours** (`art_src/palettes/sd_master.gpl`). Each sprite should use 4 to 12 colours from it, with no gradients and no anti-aliasing against transparency.
- Team colours are swapped through 4 **ramp slots** (dark, mid, light, highlight), and a player style only defines those 4 colours. This lets one tank sprite serve 6 or more player styles with no new art.
- Ground tiles have low contrast, using mid to dark values. Blocks have medium contrast, and tanks, bullets and pickups have high contrast. That order keeps gameplay objects popping out from the background.

**HD**
- Free colour, but every tileset keeps a **mood palette** of 5 to 8 key colours (`art_src/palettes/<tileset>.json`) that HD art must respect.
- Team colour is a separate **mask layer**, `<id>_mask.png`, tinted at runtime, so styles need no repaint.
- Ground saturation should stay at 60% or less of the saturation of units.

## 3. Animation budgets

Use the same frame **count** in both modes where possible, so timing configs are shared. HD may add frames only through the `fps` override in the fx config.

| Animation | SD frames | HD frames | FPS | Loop |
|---|---|---|---|---|
| Tank tracks (move) | 2 to 3 | 4 to 6 | 12 | yes |
| Tank idle (engine shake) | 0 to 2 | 2 to 4 | 6 | yes |
| Turret / muzzle flash | 2 | 3 to 4 | 24 | no |
| Bullet (flying) | 1 to 2 | 2 to 4 | 12 | yes |
| Hit spark | 3 to 4 | 6 to 8 | 24 | no |
| Explosion small | 4 to 5 | 8 to 10 | 20 | no |
| Explosion mid (tank death) | 6 | 12 to 16 | 20 | no |
| Explosion big / boss | 8 | 16 to 24 | 24 | no |
| Spawn portal | 4 | 8 | 12 | 2 loops |
| Shield / aura | 2 to 4 | 6 to 8 | 10 | yes |
| Pickup (coin spin, blink) | 4 to 6 | 6 to 8 | 10 | yes |
| Water / animated block | 2 to 4 | 4 to 8 | 4 to 6 | yes |
| Block damage stages | 3 stages (static) | 3 stages | n/a | n/a |

- Total FX length should stay at 0.6 s or less for normal explosions and 1.2 s or less for a boss explosion, so the screen stays readable.
- No FX may hide a tank for more than 0.3 s. Big explosions fade from the centre out.

## 4. Particles

Particles are config-driven (`configs/fx/*.json`, `"type": "particles"`), and their textures are tiny shared sprites.

| Budget | SD | HD |
|---|---|---|
| Particle texture | 1x1 to 4x4 px, from a palette colour | 8x8 to 32x32 soft sprites |
| Max particles per emitter | 16 | 64 |
| Max live particles on screen | 150 | 600 |
| Blend | normal (no additive glow) | additive allowed for fire, plasma and energy |
| Motion | snapped to the pixel grid, max 3 colour steps over the particle's life | free, with colour and alpha curves |

In SD mode, an emitter must look acceptable at 25% of its count, because a low quality setting scales the counts down.

## 5. Atlases and memory budgets

| Atlas group | SD max | HD max | Notes |
|---|---|---|---|
| `common` (UI, pickups, small FX, bullets) | 512x512 | 2048x2048 | always loaded |
| `players/<style>` | 128x128 | 512x512 | only when not using a palette swap |
| `enemies/<id>` | 128x128 | 512x512 (boss 1024) | loaded per level |
| `tilesets/<id>` (ground + blocks) | 256x256 | 1024x1024 | loaded per level |
| `fx/<pack>` | 512x512 | 2048x2048 | grouped by usage |
| **Total resident per level** | about 4 MB RGBA | about 48 MB RGBA | checked by `validate_content` |

- Formats: PNG in the source and in the build. RGBA8888 for HD, and RGBA4444 is allowed for SD where the palette survives. Padding is 2 px with extrude 1 to stop bleeding.
- One atlas should not mix SD and HD, and it should not mix tileset art with unit art.

## 6. Naming and files

- Source frames: `art_src/<mode>/<group>/<id>/<anim>_<NN>.png`, where mode is `sd` or `hd`. Example: `art_src/hd/fx/explosion_big/play_07.png`.
- Atlas frame names are the same in both modes: `<group>_<id>_<anim>_<NN>`. Example: `fx_explosion_big_play_07`. Static frames drop `_anim_NN`.
- Every visual id has an `art_src/<group>/<id>/brief.md` written before it is drawn. It holds a 1 to 3 line description, a size, frame counts from §3 and palette notes. This brief is the hand-off format between Iwan, the artist and the generators.

## 7. Production workflow

1. Write the brief: describe the asset in `brief.md`, using the sizes, frames and palette above.
2. Draw or generate it: SD in Aseprite against the master palette, HD painted or generated and then cleaned by hand.
3. Run `tools/build_atlases.py`, which builds both modes and fails if any budget is exceeded.
4. Run `validate_content` as part of `--test`. It checks that every config id has frames in both modes and that the frame counts match the fx configs.
5. Capture screenshots for `docs/history`.

## 8. Placeholders

The current generated placeholder atlases stay in place until each id has a final asset in both modes. After that, the placeholder for that id can be removed. A missing HD asset falls back to the SD asset, upscaled with nearest-neighbour filtering, and the fallback is logged once.
