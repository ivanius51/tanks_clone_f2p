# Code rules

## Naming (Unreal style)

- Types (classes, structs, enums, enum values), functions and methods, class fields, local variables: `PascalCase` (`class TankAi`, `void Update()`, `float ThinkTimer`, `const float Distance`). No `m` prefix on fields.
- Parameters: `In` prefix (`InDelta`, `InOwner`); output parameters (references the function fills in): `Out` (`OutHitTime`).
- Boolean fields, variables and parameters: `b` prefix (`bIsBlocked`, `bUsedFreeWeapon`, `bInVertical`); query methods: `Is`, `Has`, `Can`, `Should` (`IsBlocked()`). In C++ a field and a method can't share a name, so `b` is required.
- Getters and setters: `GetX()` / `SetX(InValue)`; fields are `private` and accessed only through them. A setter keeps the value within its valid range.
- Constants (`constexpr`): `PascalCase` (`TileSize`). Namespaces: `lowercase`. Template parameters: `T` + `PascalCase` (`TObject`).
- Overrides of engine virtual methods keep the engine names (`update`, `init`, `onEnter`, `create` in classes derived from Cocos).
- File-level globals (in an anonymous namespace): `G` prefix (`GState`, `GVerticalSync`).
- A name doesn't repeat its type name: instead of `Tank* Tank` use the role (`Player`, `Target`) or `Current`/`New` (`CurrentTank`, `NewButton = new Button()`); for a field, its role (`OwnerTeam`, `MoveSpeed`, `EnemyKind`). Otherwise the name hides the type in the whole class or function.
- Data structs (aggregates without behavior: `WeaponDef`, `LevelData`, `PlayerProfile`) have public `PascalCase` fields (booleans with `b`). Classes with behavior have only private fields with getters and setters.
- File = name of its main class. JSON keys: `snake_case` (data format, not code).
- No macros: `constexpr`, `inline`, templates. Don't commit commented-out code (§19.3).

> All code in `Classes/` was converted to this style on 2026-11-20.

## Namespaces

- Nested by layer: `tanks::model` (engine-free core), `tanks::viewmodel`, `tanks::ports`, `tanks::view::cocos` (Cocos scenes, HUD, widgets; later other views). Code that still mixes logic and engine stays in `tanks` until it is migrated (see ROADMAP.md, "Engine-independent core").
- No file-level `USING_NS_CC` or blanket `using namespace cocos2d`: engine types are written in full (`cocos2d::Node`, `cocos2d::ui::Button`).
- A `using namespace X` is allowed inside a `.cpp` or a function that uses only that one namespace, to avoid long, noisy code. Never in a header.
- Forward declarations go in the namespace that owns the type (`namespace tanks { class Tank; }` before `namespace tanks::view::cocos`).

## Includes

- Each file includes what it uses (std headers too); it never relies on `Pch.h`, which only speeds up the build. A `.cpp` doesn't repeat what its own header already includes. `size_t`, `std::move` and other basics need no extra `<cstddef>` / `<utility>`.
- Order, groups separated by a blank line, each sorted: own header (in a `.cpp`), `<std>`, `<system>` (WinAPI, order kept), engine (`"cocos2d.h"`, `"2d/..."`), project.
- `model/` files don't use the PCH, so the compiler keeps engine headers out of the Model.

## Data and containers

- Numbers that tune behavior and balance (timers, chances, distances, speeds, rewards) live in JSON configs in `Resources/configs/`, not in code. Distances in configs are in tiles (`*_tiles`); code multiplies by the tile size.
- UI layout and look are data too: sizes, spacing, paddings, offsets, font sizes, colors, animation times and steps (e.g. a volume step) live in `Resources/configs/ui/*.json`, read by a layout settings class (e.g. `MenuLayout::FromConfig`) that the ViewModel/View gets explicitly. A `constexpr` with such a value is allowed only as the fallback default for a missing key (the loader logs an error) and in unit tests; game code never reads the constant directly.
- Code keeps only geometry and format constants (e.g. the `0.5` axis threshold for a unit vector), config ids, and defaults for missing keys (and then logs an error).
- Config settings are read by a dedicated class (`TankAiSettings::Load`, `FromData`) that keeps them in fields with getters; logic doesn't dig into `ValueMap` itself.
- No C arrays: `std::array` / `std::vector` / STL algorithms; format strings with `StringUtils::format` into `std::string`; WinAPI buffers are `std::array<char, N>` with `.data()`. In-code data containers only for debugging or as local temporaries; data goes to configs. Don't duplicate enum value lists in an array: take the enum range with a `static_assert` on its order.

## Casts

- `dynamic_cast` is not used.
- Game objects: `ObjectCast<TTarget>(InObject)` (`core/ObjectCast.h`) checks the `ObjectType` tag and does a `static_cast`; a `static_assert` prevents casting to an unrelated class or a class without an `ObjectClass<T>` description. A new object class adds its own `ObjectClass` specialization.
- The factory returns the needed type directly: `ObjectsFactory::CreateFromConfig<Tank>(...)`, `FindObject<Tank>(Name)`, `ForEachObject<Tank>(...)`.
- Untagged engine types (nodes, actions) whose type is known in advance: `CheckedCast<TTarget>(InSource)`, a `static_cast` with an `assert` in Debug.
- An engine object's type is never guessed. Whoever creates the object records its type, then a `static_cast` is used:
  - factory nodes: `NodeType` is passed to setters together with the node (`SetObjectParams(Data, Node, NodeType)`);
  - scenes derive from `TypedScene<SceneKind::X>` (`scenes/TypedScene.h`): the scene kind is in its tag, and `SceneCast<TScene>(Scene)` returns `nullptr` for another or a transition scene;
  - widgets that must be found in the tree register themselves, e.g. `widgets::Button::FromNode(Node)`.

## Function parameters

- Large objects (strings, containers, `ValueMap`, `std::function`, structs over 16 bytes) are never passed by value: only `const T&`, or `T&`/`Out` if the function modifies them.
- A string the function only reads (compares, prints, searches) comes as `std::string_view`. `const std::string&` stays where the string goes on to an API that expects `std::string` (cocos2d, a `std::map`/`ValueMap` key). Never store a `string_view` in a field; to keep a string, use `emplace_back(InText)` / `std::string(InText)`.
- Simple values (`int`, `float`, `bool`, enums, pointers, small structs like `PixelSize`) are passed by value and marked `const` in the function definition: `void Update(const float InDelta)`, `Tank* const InOwner`. Top-level `const` is not written in header declarations, since it doesn't affect the signature.
- The project builds as C++17 (`/std:c++17` in the vcxproj).

## Comments (§19)

- §19.1 No profanity in comments.
- §19.2 Prefer NOT to comment: explain code with variable, function and class names and small functions (§14.1.1-§14.1.3). Fix unclear code by renaming or restructuring, not with a comment.
- Comment only the non-obvious, in one short "why" line (intent, constraint, workaround). "Non-obvious" means only:
  1. template code with many concrete arguments (what this instantiation is for);
  2. low-level code: direct memory work, pointers and bits, assembly, intrinsics;
  3. direct use of a specific external API (WinAPI, platform, SDK): non-obvious flags, parameters, call order, handling of return values.
- Everything else has no comments: gameplay logic, simple engine calls, getters and setters, control flow. When in doubt, don't comment.
- Don't write: a retelling of what the code does, step descriptions, change history ("added for X", "fix for Y").
- Don't add, reword or remove comments in code you're not otherwise changing. Bring comments in line with the rules only where the file is being edited anyway.
- §19.4 Every main class has a `/** description */` doc comment in its header.
