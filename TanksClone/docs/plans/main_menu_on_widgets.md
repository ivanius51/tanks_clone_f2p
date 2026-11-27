# Plan: main menu v3 on the widget system

Base: master (widget factory: `LayoutWidget`, `WidgetDef`, `widget_layout::Arrange`, menu files `configs/ui/menus/*.json`,
`MenuBuilder`, widget editor). The WIP branch commit `c37bca18` is the source of the parts that stay
(TankCondition, image resource kind, background jpg, strings, mockup tools); its hand-built MenuScene is dropped.

## What master has / lacks

| Have | Lack |
|---|---|
| Layouts: stack, row, column, grid, **anchor** (anchor + pivot + offset inside the parent) | Anchoring to the **visible screen** (the root is placed in the 1024x864 design area; 16:9/21:9 margins are unused) |
| Column/row `main_align` / `cross_align` (start/center/end) per container | Per-child **self-align** (`align_x`/`align_y`) overriding the parent's cross align |
| Rect part: fill, border, radius, hover/press fills | **Gradient** fills (vertical/horizontal, 2+ stops), highlight strip |
| `widgets::Button`: one look from `WidgetColors` (ButtonColor, ButtonBorder, Selection) | Button **styles** (normal / primary / danger / custom) with selectable colors and gradients; subtitle line |
| Menu files: items with action/when/debug | Per-item **style**, **subtitle**, **slot** placement (some items outside the main column: Exit at the bottom, Debug top-right) |
| Icon part (atlas frame) | **Image** part (a standalone picture by resource id) with `cover`/`contain` fit |

## Phase A: extend the widget system (each step: unit tests on `widget_layout`, widget editor rows, `--test=editor`)

1. **Screen anchoring.** `WidgetDef` root option `"screen": "visible" | "design"` (default design). A scene with a
   visible-screen root sizes it to `GetVisibleRect()` on `OnScreenLayoutChanged`; the existing anchor layout then pins
   children to screen edges (left panel at anchor [0, 0.5], debug at [1, 1], hint bar at [0.5, 0] with `fill_x`).
   New child flags `fill_x` / `fill_y` (stretch on one axis; today `fill` is both).
2. **Alignment.** Child `align_x` / `align_y` (start/center/end) overriding the container's cross align, and
   `margin` [l, t, r, b] per child. Pure `Arrange` change + unit tests.
3. **Gradients.** `ColorRef` grows into a `FillDef`: a color **or** `{ "gradient": "vertical"|"horizontal",
   "stops": [color, color, ...] }` (colors by name or literal, as now). Rect part draws it (DrawNode triangles with
   per-vertex colors, rounded corners kept). Optional `highlight`: a light strip at the top (color + height share).
   Works for fill, hover_fill, press_fill. Editor: a gradient row (direction + stop color pickers).
4. **Button styles.** `configs/ui/widgets.json` gets `"button_styles": { "normal": {...}, "primary": {...},
   "danger": {...} }`, each with fill (color or gradient), hover, pressed, disabled, border, selection border, text
   color, highlight. `widgets::Button` draws from a style (`SetStyle(id)`), default "normal" = today's look.
   Selection keeps the gold frame. Optional subtitle (second label, smaller, dim) from `SetSubtitle`.
5. **Menu items.** Menu file item keys: `"style"` (button style id), `"subtitle"` (value id bound by the scene),
   `"size"` override (height), `"slot"` (a container id in the root widget; default the menu's container). Keyboard
   order = file order across slots.
6. **Image part.** `"type": "image", "image": "image.menu_background", "fit": "cover"|"contain"|"stretch"` using the
   Image resource kind (ported from the WIP commit).

## Phase B: rebuild the main menu as data

7. `widgets/main_menu.json` (screen root, anchor layout):
   background image (cover, fill) → left panel (gradient rect, gold edge, column: title, campaign info,
   `play` slot, `list` slot, spacer grow, `bottom` slot) → garage column on the right (wallet card, garage widget)
   → `debug` slot top-right → hint bar (fill_x, bottom).
   Garage position: right-aligned with margin on wide screens, centred in free space when narrow — a widget rule
   (`anchor` + `max_width`), or one small scene hook if data cannot express it.
8. `widgets/garage.json`: caption, tank icon (scaled), damage marks (icons with `show_if: damaged`), smoke animation
   (`configs/ui/animations/smoke_rise.json`), durability bar (Bar part, fraction), condition label, Repair button —
   all `show_if: damaged`.
9. `menus/main_menu.json`: play (style primary, subtitle `next_level`), new_game (smaller, normal style), upgrades, editor, settings, quit (style danger,
   slot bottom), debug (slot debug, debug only), credits (slot credits, bottom-right), repair (slot garage, when `damaged`, enabled_when `can_repair`).
   Custom level / custom campaigns move into the editor screen (already has Play/Test).
10. `MenuScene`: only bindings (actions, conditions, values: next level name, wallet text, condition %, repair cost)
    + `TankCondition` (ported). No layout code.
11. Tests: menu tests press MENU_PLAY; editor path for the custom level; unit tests for gradient parsing, align,
    fill_x, screen root sizing, TankCondition math.
12. Showcase `--menus` at 4:3, 16:9, 21:9 (healthy + damaged profile) → screenshots to you; history notes, CHANGELOG,
    commit, merge to master.

## Order and checkpoints

A1-A2 (layout) → screenshot of the old menu pinned to screen edges → A3-A4 (gradient + button styles) → screenshot of
a button style gallery → A5-A6 → B7-B12. Each phase is one commit, built with `tools\build.ps1 -Configuration
CMakeTest` and `--test=unit` / `--test=editor` / `--test=menu`.

## Decisions (2026-11-27)

- Gradients: up to **5 stops** (`static_assert`-ed constant, can grow later); fewer is fine, min 2.
- Button styles: **JSON only**. The widget editor lists the style ids in a dropdown on a button; a button with no style,
  or the plain "default" style, shows a single **main color** picker that overrides the fill.
- Main menu items: Play (primary, subtitle = next level), then **New game** right under it (smaller height, normal
  style, no accent; asks to confirm when there is progress), Upgrades, Level editor, Settings, Exit (danger, bottom).
  **Credits**: a small button in the bottom-right corner (slot `credits`), opens the credits screen (mockup exists;
  a simple scrolling credits view from `configs/credits.json` is added in Phase B).
