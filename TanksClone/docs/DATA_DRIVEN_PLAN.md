# Data-driven TanksClone: one plan

Status: plan (2026-11-21). One document for making everything a designer, player, platform, mod or update could
want to change live in JSON. It combines the specs already decided (input actions, UI layout) with an audit of the
rest of `Classes/` and one work order.

## 1. The rule

From `.claude/rules/code-style.md`, "Data and containers":

- **No hard-coded data in game code.** Gameplay and balance numbers, input bindings, UI layout and look, resource
  paths, lists and tables, default profile values, window and display defaults are data in `Resources/configs/`,
  read by a settings class and passed in explicitly.
- A `constexpr` holding such a value is allowed **only** as the fallback default for a missing key (the loader logs
  an error) and in unit tests. Game code never reads it directly.
- Code keeps only geometry and format constants (the `0.5` axis threshold, epsilons, degrees-to-radians), config ids,
  and defaults for missing keys.
- Settings are read by a dedicated class (`XxxSettings::FromConfig(const model::IConfigNode&)`) that keeps them in
  private fields with getters; logic never digs into a config node or `ValueMap`.
- Distances in tiles use `*_tiles` keys; code multiplies by the tile size.

## 2. Shared groundwork (used by every section below)

- **Reader:** `model::IConfigNode` (`GetValue<T>`, `GetValueOr`, `TryRead`, vectors, maps, enums by name, types with
  `FromConfig`) over `core/JsonConfigNode`. New loaders use only this, never `ConfigReader`/`ValueMap`.
- **Missing keys:** `GetValueOr(Key, Default)` logs once per config and key (`config <id>: missing key <key>, using
  <default>`), so a fallback is visible. Add a `ConfigLog` helper that the run-wide "nothing missing" check at the end
  of `--test` also reads (same idea as `ResourceStatusBook`).
- **Config ids:** every config file is reached by id through `configs/resources/configs.json` (`cfg.*`), never a path
  literal. Ids live in `core/ResourceIds.h`.
- **Shared value types:** `model::Color` from `"#RRGGBB"` / `"#RRGGBBAA"` strings or `[r, g, b, a]`; `model::Vec2f`
  from `[x, y]`; `model::ScreenAnchor` by name (`"top_left"` ...). One `FromConfig` each, unit-tested once.
- **Hot reload (later):** settings objects are values, so a Debug-only "reload configs" (F5) can rebuild them and
  re-create the current screen.
- **Validation:** a unit test loads every config through its settings class and fails on any logged missing key or
  unknown enum name (covers the ROADMAP item "validator of JSON at startup").

## 3. Input actions (decided)

### 3.1 Behaviour

- `configs/input.json` lists, per **input context** (`menu`, `gameplay`, `editor`) and per **action**, the buttons
  that trigger it. A button is a keyboard key today; the format already allows a gamepad button and a player number.
- The active context is a **stack**. Gameplay pushes `gameplay`; pause and options push `menu` on top, so nothing
  reaches gameplay while they are open; closing pops back. Only the top context receives buttons.
- Lookup: a `std::array` indexed by context, each holding a map from a button to its list of actions. A button not
  bound in the active context is ignored. A bound button raises **pressed/released** events for each of its actions.
- Each action keeps its **held** state (a count of held buttons bound to it, so arrows and WASD together don't
  release early). Held state gives hold-to-fire and continuous movement. When a context is pushed or popped, every
  held action of the context that loses focus gets a release event.
- All of this is engine-free Model code (`tanks::model`), unit-tested. The Cocos keyboard adapter behind
  `ports/IInputSource` (`platform/cocos/CocosKeyboardInput`) only converts Cocos key codes into `InputButton`s.
- Debug keys (F3) live in the `debug` section and are bound only in builds with `bHasDebugTools`.

### 3.2 JSON schema and example

```json
{
	"contexts": {
		"menu": {
			"ui_up":      [ { "key": "up" }, { "key": "w" } ],
			"ui_down":    [ { "key": "down" }, { "key": "s" } ],
			"ui_confirm": [ { "key": "enter" }, { "key": "keypad_enter" }, { "key": "space" } ],
			"ui_back":    [ { "key": "escape" } ]
		},
		"gameplay": {
			"move_up":       [ { "key": "up" }, { "key": "w" } ],
			"move_down":     [ { "key": "down" }, { "key": "s" } ],
			"move_left":     [ { "key": "left" }, { "key": "a" } ],
			"move_right":    [ { "key": "right" }, { "key": "d" } ],
			"fire":          [ { "key": "space" } ],
			"ability_1":     [ { "key": "e" } ],
			"switch_weapon": [ { "key": "q" }, { "key": "tab" } ],
			"pause":         [ { "key": "escape" }, { "pad": "start", "player": 1 } ]
		},
		"editor": {
			"select_item_1": [ { "key": "1" } ],
			"select_item_9": [ { "key": "9" } ],
			"next_tab":      [ { "key": "tab" } ],
			"back":          [ { "key": "escape" } ]
		}
	},
	"debug": {
		"toggle_stats": [ { "key": "f3" } ]
	}
}
```

- Context and action names are enum names (snake_case in JSON, `InputContext`/`InputAction` in code).
- A button is an object: exactly one of `"key"` (keyboard key name) or `"pad"` (gamepad button name), optional
  `"player"` (1-based, default 1). Unknown names are logged and skipped, the rest still loads.
- The HUD reads the bound key names for its hotkey labels (`fire` → "SPACE", `ability_1` → "E",
  `switch_weapon` → "Q") through `InputBindings::GetButtons(Context, Action)`, so rebinding updates the HUD.
- A player rebinding file later (`input_user.json` in the storage port) overrides whole actions by name.

### 3.3 C++ types (`model/Input*.h`)

```cpp
namespace tanks::model
{
enum class InputContext { Menu, Gameplay, Editor, Debug, Count };

enum class InputAction
{
	UiUp, UiDown, UiConfirm, UiBack,
	MoveUp, MoveDown, MoveLeft, MoveRight, Fire, Ability1, SwitchWeapon, Pause,
	SelectItem1, /* ... */ SelectItem9, NextTab, Back,
	ToggleStats,
	Count
};

enum class InputDevice { Keyboard, Gamepad };

/** One physical button: a key today, a gamepad button of a given player later. */
struct InputButton
{
	InputDevice Device = InputDevice::Keyboard;
	int Code = 0;        // model::Key for the keyboard, model::PadButton for a gamepad
	int Player = 1;
	bool operator==(const InputButton&) const; bool operator<(const InputButton&) const;
};

struct InputEvent { InputAction Action; bool bPressed; };

/** Which buttons trigger which actions, per context. Read from configs/input.json. */
class InputBindings
{
public:
	static InputBindings FromConfig(const IConfigNode& InNode);
	const std::vector<InputAction>& Find(InputContext InContext, const InputButton& InButton) const;
	std::vector<InputButton> GetButtons(InputContext InContext, InputAction InAction) const;
private:
	std::array<std::map<InputButton, std::vector<InputAction>>, static_cast<size_t>(InputContext::Count)> Contexts;
};

/** The active input contexts, top one receives buttons; tracks which actions are held. */
class InputContextStack
{
public:
	explicit InputContextStack(const InputBindings& InBindings);
	void Push(InputContext InContext);   // releases held actions of the previous top
	void Pop();                          // releases held actions of the popped context
	InputContext GetActive() const;
	void OnButton(const InputButton& InButton, bool bInPressed, std::vector<InputEvent>& OutEvents);
	bool IsHeld(InputAction InAction) const;
private:
	const InputBindings& Bindings;
	std::vector<InputContext> Stack;
	std::array<int, static_cast<size_t>(InputAction::Count)> HeldCount{};
};
}
```

- `model::Key` (`model/InputTypes.h`) grows to the full keyboard set the configs can name (letters, digits, F-keys,
  arrows, Enter, keypad Enter, Escape, Tab, Space, Shift, Ctrl); key names map to it through one name table used by
  both the loader and the HUD labels.
- `PlayerControls` consumes `InputEvent`s and `IsHeld` instead of keys; `widgets::ButtonList` consumes `UiUp/UiDown/
  UiConfirm/UiBack`; `EditorScene` consumes `SelectItemN/NextTab/Back`; `AppDelegate` F3 → `ToggleStats`.
- `InputSettings` (owner of `InputBindings` + `InputContextStack`) is a manager in `GameContext::GameManagers`,
  after the port adapters.

### 3.4 Unit tests

1. Loads the example: every action of every context has its buttons; unknown key name is logged and skipped.
2. Button not bound in the active context → no event.
3. Bound button → pressed event for each of its actions (one button, two actions).
4. Release → released event; two buttons on one action: releasing one keeps it held.
5. Hold-to-fire: `IsHeld(Fire)` stays true across frames until release.
6. Push `menu` while `fire` is held → `fire` released; gameplay buttons give nothing while `menu` is on top.
7. Pop back to `gameplay` → gameplay buttons work again; nothing is "stuck" held.
8. Same key in two contexts (`escape` = `pause` in gameplay, `ui_back` in menu) resolves by the top context.
9. Gamepad button with `player: 2` doesn't trigger player 1's binding.
10. `GetButtons` order matches the config (HUD shows the first one).
11. Debug section is ignored when debug tools are off.

## 4. UI layout (decided)

- Every UI size, spacing, padding, offset, font size, color, animation time and step (such as `volume_step`) lives in
  `configs/ui/*.json`.
- One settings class per file (e.g. `MenuLayout::FromConfig`, `HudLayout::FromConfig`, `WidgetStyle::FromConfig`),
  reading through `IConfigNode`, holding values in fields with getters. It is created once (a `UiStyle` manager in
  `GameContext` owns all of them) and passed explicitly to the ViewModel/View constructors; scenes don't read
  configs themselves. The current `constexpr`s become the fallback defaults (logged when used).
- Text-size factors (Small/Medium/Large) come from `configs/ui/widgets.json` `"text_scales"` (today
  `configs/display.json`; `model::TextScales::FromConfig` already exists).
- HUD placement for any aspect ratio: each HUD block names a `model::ScreenAnchor` and an offset from it
  (`model::ScreenLayout::Place`); anchors and offsets are config values, not code.
- Colors are `"#RRGGBB(AA)"` strings; named palette entries (`"accent"`) may be used wherever a color is expected.

### 4.1 File split and keys

`configs/ui/widgets.json` (shared look, `WidgetStyle`):

```json
{
	"text_scales": [0.9, 1.0, 1.12],
	"fonts": { "decorative_scale": 0.55, "system_fallback": "Arial" },
	"palette": {
		"text": "#EBEBEB", "text_dim": "#9696A0", "accent": "#FFCD46", "good": "#6EDC6E", "bad": "#EB5A50",
		"panel": "#14171FF5", "panel_border": "#737F99", "button": "#293042", "button_pressed": "#1A1F2B",
		"button_disabled": "#1C1F26", "button_border": "#5A6680", "selection": "#FFCC40",
		"screen_background": "#0E1118", "grid_line": "#FFFFFF0A", "dim": "#000000AA", "outline": "#000000"
	},
	"button": { "height": 52, "border": 2, "font_size": 24 },
	"overlay": { "width": 640, "padding": 28, "button_spacing": 12, "title_font": 40, "message_font": 22,
	             "message_gap": 20, "buttons_gap": 24 },
	"label_outline": 2,
	"toast": { "font_size": 26, "show_time": 2.0, "fade_time": 0.4 }
}
```

`configs/ui/menu.json` (`MenuLayout`): main menu, settings panel, upgrades, loading screen.

```json
{
	"main": { "title_font": 84, "info_font": 22, "button_size": [380, 52], "button_spacing": 12,
	          "list_center_offset": [0, -60] },
	"settings": { "title_font": 56, "button_size": [560, 48], "button_spacing": 8, "list_center_offset": 40,
	              "title_top_offset": 120, "panel_padding": 28, "title_gap": 24, "volume_step": 20 },
	"upgrades": { "title_font": 56, "name_font": 28, "info_font": 18, "first_row_top": 235, "row_step": 100,
	              "name_x": 110, "name_offset_y": 12, "info_offset_y": -18, "pips_x": 520, "pip_size": 18,
	              "pip_step": 26, "buy_x": 850, "buy_size": [220, 56],
	              "pip_empty": "#1C1C26", "pip_full": "#FFCC40", "pip_border": "#59667F" },
	"loading": { "title_font": 84, "status_font": 24, "min_show_time": 0.6, "bar_size": [480, 16], "bar_border": 2,
	             "step_delay": 0.05 }
}
```

`configs/ui/hud.json` (`HudLayout`): top strip and bottom panel, each block anchored.

```json
{
	"top": { "anchor": "top_left", "height": 48, "padding": 16, "font": 20, "small_font": 18,
	         "wave_x": 300, "enemies_x": 470, "enemies_bar": { "x": 640, "size": [130, 12] } },
	"bottom": { "anchor": "bottom_left", "height": 112,
	            "portrait": { "x": 16, "size": 88, "icon_inset": 20 },
	            "bars": { "x": 124, "width": 300, "health": [74, 24], "ammo": [46, 20], "reload": [36, 5],
	                      "stats_y": 17, "stats_font": 17, "health_low_percent": 30 },
	            "slots": { "anchor": "bottom", "x": 506, "step": 88, "size": 72, "icon_size": 44, "max": 3,
	                       "hotkey_font": 17, "hotkey_font_long": 13, "switch_key_font": 15, "timer_font": 26,
	                       "timer_min_seconds": 0.05 } },
	"banner": { "font": 48, "fade_in": 0.3, "hold": 1.4, "fade_out": 0.5 },
	"colors": { "panel": "#121419", "bar_background": "#292B36", "health": "#4DCC4D", "health_low": "#E64D40",
	            "ammo": "#F2B333", "ammo_empty": "#CC4033", "reload": "#E6E6E6", "enemies": "#D9594D",
	            "slot": "#1A1C26", "slot_ready_border": "#F2CC4D", "cooldown_shade": "#000000AD" }
}
```

`configs/ui/editor.json` (`EditorStyle`, feeding `model::EditorLayout`):

```json
{
	"layout": { "padding": 8, "gap": 6, "max_tab_height": 30, "max_item_size": 48, "min_item_size": 12,
	            "max_action_panel_width": 280, "action_panel_share": 0.4, "action_columns": 2, "icon_fill": 0.8 },
	"enemy_label_font": 22,
	"colors": { "toolbar": "#141C1C", "panel": "#1C1F29", "erase": "#E65A4D" }
}
```

`configs/ui/game.json` (world-space look that is not HUD): shield bubble (`radius_px` 25, three colors), effect scale
(2.0), explosion frame delay (0.05), bonus pulse (0.5 s, ×1.15), background floor colors and frame color
(`ui/Background.cpp`, merge with `configs/background.json`).

`configs/ui/splash.json` (read by the Win32 splash before the engine: title, status text id, font face, size
divisors, width) — only if the running session doesn't already put these in `display.json`.

## 5. Audit of everything else in `Classes/` (2026-11-21, commit `bd00d1c`)

Line numbers are from that commit; the running UI session moves some of them. "Keep" = geometry/format constant or
an id, allowed by the rule.

### 5.1 Balance and timings

| file:line | value | target config / key |
|---|---|---|
| `scenes/GameScene.cpp:39` | `KillScore = 100` (fallback when the killed object is not a tank) | `configs/rules.json` `kill_score` |
| `scenes/GameScene.cpp:40` | `KillCoins = 10` | `rules.json` `kill_coins` |
| `scenes/GameScene.cpp:41,412` | `WinCoinsPerLevel = 50` × (level + 1) | `rules.json` `win_coins_per_level` (or per level `reward_coins`) |
| `scenes/GameScene.cpp:42-43` | result delay win 1.0 s, lose 1.4 s | `rules.json` `result_delay_win`, `result_delay_lose` |
| `scenes/GameScene.cpp:332` | banner fade 0.3 / hold 1.4 / fade 0.5 | `ui/hud.json` `banner.*` |
| `game/EnemySpawner.cpp:18` | spawn flash 0.5 s | `rules.json` `spawn.flash_time` |
| `game/EnemySpawner.cpp:22` | free-area margin 6 px | `rules.json` `spawn.free_margin_tiles` |
| `game/LevelDataLoader.cpp:12` | default wave delay 1.0 s | `rules.json` `level_defaults.wave_delay` |
| `game/LevelLoader.cpp:20` | default enemy `"Basic"` | `rules.json` `level_defaults.enemy` |
| `scenes/EditorScene.cpp:30-32` | enemies 1..40, max alive 4 | `ui/editor.json` `limits.{min_enemies,max_enemies,max_alive}` |
| `game/objects/MovableObject.cpp:25` | max sub-step 8 px | keep (simulation step, geometry) |
| `game/objects/MovableObject.cpp:29` | default corner assist = tile/3 | already JSON `corner_assist`; default → `rules.json` `movement.corner_assist_tiles` |
| `game/objects/MovableObject.cpp:32-35` | turn 0.12 s, ease 2.0, anim speeds 1.0/1.5 | tank config `turn_time`, `turn_ease`; anim speeds → `ui/game.json` |
| `game/objects/Tank.cpp:18` | muzzle distance 0.6 tile | tank config `muzzle_tiles` |
| `game/components/Ability.cpp:145` | ability cooldown default 10 s | fallback only (ability config `cooldown`), log when missing |
| `game/objects/TankLoader.cpp:13-15` | default weapon icon/name, primary ability `"Shoot"` | `rules.json` `weapon_defaults` |
| `game/Effects.cpp:16-19` | explosion frame delay 0.05, scale 2.0, frame counts 5/3 | `ui/game.json` `effects.*`; frame counts from the atlas (count frames) |
| `game/ObjectsFactory.cpp:23` | animation frame delay 0.15 | object config `frame_delay` (default in `rules.json`) |
| `game/objects/Bonus.cpp:26-27` | pulse 0.5 s, ×1.15 | `ui/game.json` `bonus_pulse` |
| `services/DisplaySettings.h:54` | menu FPS 60 | `display.json` `menu_fps` |
| `services/DisplaySettings.cpp:34,39` | auto window 0.9 of desktop, splash width 480 | `display.json` `auto_screen_share`, `splash_width` |
| `services/ResourceManager.cpp:21-22` | placeholder 16 px, cell 4 | keep (debug placeholder format) |
| `core/GameConstants.h:15-23` | tile 64, grid 16×11, HUD heights 112/48, design size | tile + grid → level format / `rules.json` `world.tile_size`; HUD heights → `ui/hud.json`; design size → `display.json` `design_size` (scrolling makes grid per level anyway) |
| `game/objects/Bullet.cpp:24,26` | chip depth unit tile/4, contact slack 1 px | keep (wall cell geometry) |

Already data: enemy stats, AI (`configs/ai/*.json`), buffs, upgrades costs, bullets, walls, bonuses, levels.

### 5.2 Remaining key codes

| file:line | value | target |
|---|---|---|
| `AppDelegate.cpp:69` | `KEY_F3` toggles stats | `input.json` `debug.toggle_stats` |
| `game/PlayerControls.cpp:15-28,66,74,78,124` | arrows/WASD, Space, E, Q/Tab | `input.json` `gameplay.*` |
| `ui/Widgets.cpp:300-310` | menu navigation keys | `input.json` `menu.ui_*` |
| `scenes/EditorScene.cpp:124-128` | 1-9, Tab | `input.json` `editor.*` |
| `platform/cocos/CocosKeyboardInput.cpp:43,51-70` | Cocos → `model::Key` map | stays (adapter's job); grows to the full key set |
| `ui/Hud.cpp:289,300` | `"Q"`, `"SPACE"`, `"E"` labels | from `InputBindings::GetButtons` + key display names |

### 5.3 UI constants (→ section 4 files)

| file:line | constants | target |
|---|---|---|
| `ui/Widgets.h:17-28` | 12 palette colors | `ui/widgets.json` `palette` |
| `ui/Widgets.cpp:23-33` | border 2, button height 52, overlay 640/28/12, fonts 24/40/22, grid step, decorative scale 0.55 | `ui/widgets.json` `button`, `overlay`, `fonts` |
| `ui/Widgets.cpp:69` | system font `"Arial"` | `resources/fonts.json` `font.system` |
| `ui/Widgets.cpp:91,100,443` | screen background, grid line, dim 170 | `ui/widgets.json` `palette` |
| `ui/Widgets.cpp:479-480` | gaps 20 / 24 | `ui/widgets.json` `overlay.message_gap`, `buttons_gap` |
| `ui/SettingsPanel.cpp:17-25` | title 56, button 560×48, spacing 8, offsets 40/120, padding 28, gap 24, `VolumeStep` 20 | `ui/menu.json` `settings.*` |
| `scenes/MenuScene.cpp:25-29,88,100` | fonts 84/22, button 380×52, spacing 12, overlay z 100, list offset −60 | `ui/menu.json` `main.*` |
| `scenes/UpgradesScene.cpp:21-32,83,88,97-99` | fonts, rows, pips, buy button, offsets ±12/18, pip colors | `ui/menu.json` `upgrades.*` |
| `scenes/LoadingScene.cpp:27-33,81,156-159` | fonts 84/24, min show 0.6, bar 480×16/2, step delay 0.05 | `ui/menu.json` `loading.*` (`LogSteps` keep: debug log format) |
| `ui/Hud.cpp:21-47,49-61,222,237,289-307,330` | ~25 sizes/positions, 11 colors, fonts 15/13/17/26/17, inset 20, low-health 30 % | `ui/hud.json` |
| `scenes/EditorScene.cpp:28-29,37-40,447-451` | icon fill, label font, 4 colors, toast 26 / 2.0 / 0.4 | `ui/editor.json`, `ui/widgets.json` `toast` |
| `model/EditorLayout.cpp:10-17` | padding 8, gap 6, tab 30, items 12..48, panel 280 / 0.4, 2 columns | `ui/editor.json` `layout` (pass an `EditorLayoutSettings` struct in) |
| `scenes/GameScene.cpp:44` | banner font 48 | `ui/hud.json` `banner.font` |
| `game/objects/Tank.cpp:21,133-135` | shield radius 25, three colors, 36 segments | `ui/game.json` `shield` |
| `ui/Background.cpp:19-21` | floor A/B, frame colors | `configs/background.json` |
| `scenes/EditorScene.cpp:449`, `ui/Hud.cpp:292,303,307` | outline black, width 2 | `ui/widgets.json` `palette.outline`, `label_outline` |
| `platform/SplashWindow.cpp:11-21` | title, status, font face, size divisors | `ui/splash.json` or `display.json` `splash` |
| `core/GameConstants.h:26-31`, `scenes/*:Z*` | z-orders | keep (draw order is code structure, not tuning) |

### 5.4 Resource paths

| file:line | literal | target id |
|---|---|---|
| `core/GameConstants.h:9` | `configs/resources.json` | keep (root of the id table, the one bootstrap path) |
| `core/GameConstants.h:10-11` | `configs/`, `configs/enemies/` | `resources/configs.json` folders `cfg_folder.enemies` |
| `game/ai/AiSettingsLoader.cpp:13-14` | `configs/ai/tank_ai.json`, `player_bot.json` | `cfg.tank_ai`, `cfg.player_bot` |
| `game/components/Buff.cpp:16` | `configs/buffs.json` | `cfg.buffs` |
| `scenes/EditorScene.cpp:26` | `configs/editor/palette.json` | `cfg.editor_palette` |
| `services/DisplaySettings.cpp:30` | `configs/display.json` | `cfg.display` (splash reads it before the engine: keep a bootstrap path there only) |
| `services/StringsManager.cpp:11-13` | three strings files | `configs/strings/languages.json` (see 5.5) |
| `ui/Background.cpp:23` | `configs/background.json` | `cfg.background` |
| `game/LevelLoader.cpp:41` | `configs/levels/level_<n>.json` built by code | `configs/campaign.json` list of level ids (see 5.5) |
| `game/LevelLoader.cpp:18-19`, `services/ProfileManager.cpp:15`, `platform/cocos/LegacyProfile.cpp:12-13` | storage file names | keep (storage-port names, not resources) |
| `ui/Hud.cpp:222` | frame `"player_tank"` | player tank config `portrait` |
| `game/components/Ability.cpp`/`TankLoader.cpp:13` | icon `"icon_shoot"` | `rules.json` `weapon_defaults.icon` |
| `model/EditorPalette.cpp:9` | `"erase"` icon | `editor/palette.json` `erase_icon` |
| `core/ResourceIds.h:12-23` | sound ids for bonus, wall, bullet hits, game flow | move to the configs that cause them (bonus/wall/bullet `sound_*`, `rules.json` `sounds.*`), as step 2b did for tanks |

### 5.5 In-code tables and lists

| file:line | table | target |
|---|---|---|
| `scenes/MenuScene.cpp:72-98` | main menu entries and their order | `ui/menu.json` `main.items` (`continue`, `new_game`, `upgrades`, `editor`, `custom_level`, `settings`, `quit`: ids map to commands in code; order/visibility in data) |
| `ui/SettingsPanel.cpp:85-116` | settings entries and order | `ui/menu.json` `settings.items` |
| `ui/SettingsPanel.cpp:39-44` | text size → string id | `ui/widgets.json` `text_sizes: [{ "scale":0.9, "name":"TEXT_SIZE_SMALL" }, ...]` (merge with `text_scales`) |
| `scenes/EditorScene.cpp:194-197` | editor action buttons | `ui/editor.json` `actions` |
| `services/StringsManager.cpp:19-21`, `StringsManager.h:11-16` | languages (enum + files) | `configs/strings/languages.json` `[{ "id":"en", "file":"...", "name":"LANGUAGE_NAME" }]`; `Language` enum → string id; settings cycles the list |
| `game/LevelLoader.cpp:41` | campaign = `level_1..N` | `configs/campaign.json` `levels: [...]` |
| `game/LevelLoader.h:15-22` | map tile characters | `configs/level_format.json` `tiles: { "#": "wall.brick", ... }` (the editor palette already names objects) |
| `game/components/Ability.cpp:164-165`, `Stats.cpp:17-22`, `ai/AiSettingsLoader.cpp:21-25` | name ↔ class/enum tables | keep (code registry of types configs refer to) |
| `services/DisplaySettings` | fps / window scale options | already in `display.json` |

### 5.6 Default profile and display values

| file:line | value | target |
|---|---|---|
| `services/ProfileManager.h:17-31` | coins 0, language English, sound on, FPS 60, vsync on, windowed, scale 100, HD on, smooth off, volume 100, text Small | `configs/profile_defaults.json` (read into a `PlayerProfile` by the serializer; a new profile = defaults) |
| `core/GameConstants.h:7` | player name `"PLAYER"` | `profile_defaults.json` `player_name` |
| `AppDelegate.cpp:28` | window title | `display.json` `window_title` (string id) |

### 5.7 Player-visible strings that bypass `StringsManager`

| file:line | text | target |
|---|---|---|
| `scenes/LoadingScene.cpp:61` | `"TANKS CLONE"` | `Text("GAME_TITLE")` |
| `scenes/LoadingScene.cpp:65` | `"Loading..."` | `Text("LOADING")` |
| `platform/SplashWindow.cpp:12-13` | `L"TANKS CLONE"`, `L"Loading..."` | strings file read before the engine (language from profile), or keep as the one pre-config exception |
| `ui/Hud.cpp:289,300` | `"Q"`, `"SPACE"`, `"E"` | key display names (`KEY_SPACE`) from bindings |
| `ui/Hud.cpp:339` | `"INF"` | `Text("AMMO_INFINITE")` |
| `scenes/MenuScene.cpp:57`, `UpgradesScene.cpp:65`, `EditorScene.cpp:410`, `SettingsPanel.cpp:94-109` | `": "`, `" / "`, `"%"` glued in code | format strings in the strings file (`"HUD_SCORE_FMT": "Score: {0}"`) and a small `Format(Id, Args...)` |
| `game/objects/Tank.cpp:218` | `"Player"/"Enemy"/"Neutral"` | not shown to the player (config override): keep |

## 6. Work order

One order for all of it. Size: S ≈ under half a day, M ≈ a day, L ≈ several days.
**[UI session]** marks steps the running session "Splash window, aspect ratios, font sizes, pause/back,
hold-to-fire" is already implementing; this plan doesn't redo them, later steps build on what it lands.

1. **Groundwork** (M): `GetValueOr` logging + `ConfigLog`; `model::Color` / `Vec2f` / `ScreenAnchor` `FromConfig`;
   every config path literal → `cfg.*` id (5.4); config validation unit test. Blocks everything below.
2. **Input actions** (M) **[UI session]**: `configs/input.json`, `InputBindings`, `InputContextStack`, adapter
   converts to buttons, pause/options push `menu`, hold-to-fire, tests of 3.4. Follow-up here only: F3 into
   `debug`, editor keys, HUD hotkey labels from bindings (S).
3. **Text sizes** (S) **[UI session]**: `TextScales` from config, `TextSize` in the profile and settings. Follow-up:
   move `text_scales` into `ui/widgets.json` with the size names (S).
4. **Aspect ratios / HUD anchors** (M) **[UI session]**: `model::ScreenLayout`, `ScreenAnchor`, HUD floats to
   edges. Follow-up: anchors and offsets read from `ui/hud.json` (part of step 5).
5. **`configs/ui` files and layout classes** (L) **[UI session starts it]**: `widgets.json` + `WidgetStyle` first
   (everything uses it), then `menu.json` (`MenuLayout`, settings panel incl. `volume_step`), `hud.json`
   (`HudLayout`), `editor.json` (`EditorLayout` gets its settings struct), `game.json`. Check with the UI session
   which files it has landed before starting; take the rest of 5.3 file by file.
6. **Strings** (S-M): 5.7 literals to string ids, format strings, `languages.json` (5.5). After 5 so labels are
   touched once.
7. **Menus and lists as data** (M): main menu, settings entries, editor actions, campaign list, level tile chars
   (5.5). After 5 and 6.
8. **Balance and rules** (M): `configs/rules.json` + `RulesSettings` (5.1), remaining sounds into data (5.4 last
   row). Independent of 2-7; can run in parallel with them (touches game code, not UI).
9. **Profile and display defaults** (S): `profile_defaults.json`, window title, menu FPS, auto share (5.6). After
   the UI session lands (it edits `DisplaySettings` and the profile).
10. **Sweep** (S): grep for `constexpr float|int`, color literals, `"...json"`, `KeyCode::` in `Classes/`; every hit
    is a fallback default (logged), a test value or a "keep" from section 5. Update this document's tables to done.

Tests per step: unit tests for each settings class (defaults when a key is missing, values when present, logged
miss); `--test=menu` for steps 2-7 and 9; `--test` for 1 and 8.
