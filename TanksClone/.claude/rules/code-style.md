# Code rules

## Naming (Unreal style)

- Types (classes, structs, enums, enum values), functions and methods, class fields, local variables: `PascalCase` (`class TankAi`, `void Update()`, `float ThinkTimer`, `const float Distance`). No `m` prefix on fields.
- Parameters: `In` prefix (`InDelta`, `InOwner`); output parameters (references the function fills in): `Out` (`OutHitTime`).
- Boolean fields, variables and parameters: `b` prefix (`bIsBlocked`, `bUsedFreeWeapon`, `bInVertical`); query methods: `Is`, `Has`, `Can`, `Should` (`IsBlocked()`). In C++ a field and a method can't share a name, so `b` is required.
- Getters and setters: `GetX()` / `SetX(InValue)`; fields are `private` and accessed only through them. A setter keeps the value within its valid range.
- Constants (`constexpr`): `PascalCase` (`TileSize`). Namespaces: `lowercase`. Template parameters: `T` + `PascalCase` (`TObject`). Template classes also take the `T` prefix (`TFunction`, `TArray`, `TTypeList`); the file has the same name.
- Overrides of engine virtual methods keep the engine names (`update`, `init`, `onEnter`, `create` in classes derived from Cocos).
- File-level globals (in an anonymous namespace): `G` prefix (`GState`, `GVerticalSync`).
- A name doesn't repeat its type name: instead of `Tank* Tank` use the role (`Player`, `Target`) or `Current`/`New` (`CurrentTank`, `NewButton = new Button()`); for a field, its role (`OwnerTeam`, `MoveSpeed`, `EnemyKind`). Otherwise the name hides the type in the whole class or function.
- Data structs (aggregates without behavior: `WeaponDef`, `LevelData`, `PlayerProfile`) have public `PascalCase` fields (booleans with `b`). Classes with behavior have only private fields with getters and setters.
- File = name of its main class. JSON keys: `snake_case` (data format, not code).
- No macros: `constexpr`, `inline`, templates. Don't commit commented-out code (§19.3).

## Formatting and flow

- Brace initialization `{}` instead of `=` where it gives a stricter type check (`const int Count{ 0 };`, `std::array<Kind, 2> Kinds{ A, B };`): narrowing is then a compile error.
- Keep the logic flat; don't nest. Two tools:
  - Guard clauses: check errors and edge cases first and leave at once with `return` or `continue`; no `else` after them, so the main logic stays at the left margin.
  - Extraction: when a block still goes deep (complex logic inside a loop), move it into a small, clearly named function.
- Every scope has braces, including one-line `if`, `for` and `while`.
- Every brace is on its own line. Exceptions: an empty body and a one-line implementation in a header (`int GetCount() const { return Count; }`).

## Namespaces

- Nested by layer: `game::model` (engine-free core), `game::viewmodel`, `game::ports`, `game::view::cocos` (Cocos scenes, HUD, widgets; later other views). Code that still mixes logic and engine stays in `game` until it is migrated (see ROADMAP.md, "Engine-independent core").
- No file-level `USING_NS_CC` or blanket `using namespace cocos2d`: engine types are written in full (`cocos2d::Node`, `cocos2d::ui::Button`).
- A `using namespace X` is allowed inside a `.cpp` or a function that uses only that one namespace, to avoid long, noisy code. Never in a header.
- Forward declarations go in the namespace that owns the type (`namespace game { class Tank; }` before `namespace game::view::cocos`).

## Includes

- Each file includes what it uses (std headers too); it never relies on `Pch.h`, which only speeds up the build. A `.cpp` doesn't repeat what its own header already includes. `size_t`, `std::move` and other basics need no extra `<cstddef>` / `<utility>`.
- Order, groups separated by a blank line, each sorted: own header (in a `.cpp`), `<std>`, `<system>` (WinAPI, order kept), engine (`"cocos2d.h"`, `"2d/..."`), project.
- `model/` files don't use the PCH, so the compiler keeps engine headers out of the Model.
- In a header prefer a forward declaration to an `#include` when only a pointer, a reference or a name is needed; the full include goes in the `.cpp`.

## Data and containers

- **No hard-coded data in game code.** Whatever a designer, player, platform, mod or update could change is JSON in `Resources/configs/`, read by a settings class and passed in explicitly: balance (timers, chances, speeds, rewards; distances in tiles `*_tiles`), input bindings (`configs/input.json`, no key codes in logic), UI layout and look (`configs/ui/*.json`), resource paths (by id, `configs/resources.json`), lists and tables (menus, palettes, enemy sets, defaults).
- A `constexpr` with such a value is allowed only as the fallback for a missing key (the loader logs an error) and in unit tests. Code keeps only geometry and format constants, config ids and those fallbacks.
- Settings are read by a dedicated class (`TankAiSettings::Load`, `FromData`) into fields with getters; logic never digs into `ValueMap`.
- Containers come from `templates/TContainers.h`, not `std::vector` / `std::map` / `std::unordered_map` (usage: `templates/README.md`):
  - `TArray<T>`: vector over `std::pmr`; the arena of the object's lifetime (Frame, Level, Session) is passed as `memory_resource*`. Methods: `PushBack`, `EmplaceBack`, `RemoveAt`, `RemoveAtSwap`, `Size`, `IsEmpty`, `Front`, `Back`.
  - `TMap<K, V>`: sorted flat map, iterates in key order (stable, needed for determinism); initializer lists are allowed. With a `FPersistentString` key, `Find(std::string_view)` only looks the name up and never interns it.
  - `TSmallArray<T, InlineCapacity>`: up to `InlineCapacity` values inline, spills to a `pmr` arena beyond that; for short lists on a hot path.
- Call `Reserve` when the size is known or can be estimated.
- Fixed pools and queues: `TObjectPool`, `TRingBuffer`; items that others point to: `TSlotMap` with a `TEntityHandle` (a stale handle stops resolving).
- Arrays indexed by an enum: `TEnumArray`; sets of flags: `TEnumFlags`. Don't duplicate enum lists in an array: use the enum range with a `static_assert` on its order.

## Own templates (`Classes/templates/`)

- Engine-free base templates live in `Classes/templates/` (namespace `game`, `T` prefix, one class per file); the Model may use them.
- Callbacks: no `std::function` in new code. `TFunctionRef<Sig>` for a callback used only during the call; `TFunction<Sig, InlineSize>` for a stored one (capture inline, `static_assert` if too big, never the heap); `TDelegate` / `TMulticastDelegate` for object + member handlers.
- Strings: `std::string_view` to read. A name that is stored, compared often or a key is a `TStoredString` (interned, 4 bytes, integer compare and hash, `View()` gives the text); in code it is `constexpr`, from configs it is interned at load.
- Shared assets: `THandle<T>` (`model/RefCounted.h`, ref-counted, `THandle<T>::Create<T>(Args...)` or `MakeHandle<T>(Args...)`) instead of a raw owning pointer.
- Change notification and cleanup: `TObserved<T>`, `TScopeGuard` / `TScopeExit`; distinct id types: `TStrongType`.
- Own containers beyond the ones above only where a profile shows the need.
- Per-frame hot paths (step, AI, collision) don't use the general heap: reuse buffers or a frame arena.
- Planned list: ROADMAP.md, "Own base library".

## Casts

- `dynamic_cast` is not used.
- Game objects: `ObjectCast<TTarget>(InObject)` (`core/ObjectCast.h`) checks the `EObjectType` tag and does a `static_cast`; a `static_assert` prevents casting to an unrelated class or a class without an `ObjectClass<T>` description. A new object class adds its own `ObjectClass` specialization.
- The factory returns the needed type directly: `ObjectsFactory::CreateFromConfig<Tank>(...)`, `FindObject<Tank>(Name)`, `ForEachObject<Tank>(...)`.
- Untagged engine types (nodes, actions) whose type is known in advance: `CheckedCast<TTarget>(InSource)`, a `static_cast` with an `assert` in Debug.
- An engine object's type is never guessed. Whoever creates the object records its type, then a `static_cast` is used:
  - factory nodes: `ENodeType` is passed to setters together with the node (`SetObjectParams(Data, Node, ENodeType)`);
  - scenes derive from `TypedScene<SceneKind::X>` (`scenes/TypedScene.h`): the scene kind is in its tag, and `SceneCast<TScene>(Scene)` returns `nullptr` for another or a transition scene;
  - widgets that must be found in the tree register themselves, e.g. `widgets::Button::FromNode(Node)`.

## Function parameters

- Large objects (strings, containers, `ValueMap`, `TFunction`, structs over 16 bytes) are never passed by value: only `const T&`, or `T&`/`Out` if the function modifies them.
- A string the function only reads comes as `std::string_view`; `const std::string&` stays where it goes on to an API that needs `std::string` (cocos2d, map keys). Never store a `string_view` in a field.
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
