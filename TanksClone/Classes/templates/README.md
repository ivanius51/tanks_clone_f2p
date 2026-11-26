# templates

Engine-free base templates. The Model may use them; they include no Cocos headers. One class per file (in some headers, several small helpers share a file, see below). Names: `T` prefix for templates, `F` prefix for value-like handles and strings, `In` / `Out` on parameters (see `.claude/rules/code-style.md`).

Namespace: most headers are in `game`; `TFunction.h`, `TContainers.h` and `TSlotMap.h` are in the global namespace (`TSmallArray`, `TArray`, `TMap`, `TFunction`, `TEntityHandle`).

## Callbacks

### TFunction.h — `TFunction<Sig, InlineSize>`, `TFunc<Sig>`

Owning callable with inline storage. Never allocates on the heap; a capture bigger than `InlineSize` is a `static_assert` error.

```cpp
TFunction<void(int)> OnHit = [this](const int InDamage) { Health -= InDamage; };  // default 64 bytes
TFunction<void(), UiCallbackInlineSize> OnClick = ...;                             // 128 bytes, UI only
OnHit(5);
if (OnHit) { ... }
```

- `DefaultFunctionInlineSize = 64`. `UiCallbackInlineSize = 128` is set explicitly for UI callbacks.
- Do not raise a size for a single type unless it is really needed. First shrink the capture: store names as `FPersistentString`, and put several nested callbacks into one `std::make_shared<const ...>` handle (the capture is then one pointer).

### TFunctionRef.h — `game::TFunctionRef<Sig>`

Non-owning view of a callable. Use only for a callback that is called during the call itself (a parameter), never stored.

```cpp
void ForEachTank(TFunctionRef<void(Tank&)> InVisit);
ForEachTank([](Tank& InTank) { InTank.Heal(); });
```

## Containers

### TContainers.h

- **`TArray<T, Alloc>`** — vector over `std::pmr`; pass the arena of the object's lifetime (Frame, Level, Session) as `memory_resource*`. `PushBack`, `EmplaceBack`, `PopBack`, `RemoveAt`, `RemoveAtSwap`, `Reserve`, `Resize`, `Clear`, `Front`, `Back`, `GetData`, `IsEmpty`.
- **`TMap<K, V, Compare>`** — sorted flat map with stable iteration (needed for determinism). `Contains`, `Remove`, `Reserve`, `Clear`, `IsEmpty`; initializer lists are allowed. Iterates in key order.
- **`TSet<K, Compare>`** — sorted flat set with stable iteration (replaces `std::set` and `std::unordered_set`). `Insert` (false for a duplicate), `Contains`, `Remove`, `Reserve`, `Clear`, `IsEmpty`; `Contains(std::string_view)` on an `FPersistentString` set never interns the name.
- **`TSmallArray<T, InlineCapacity>`** — stores up to `InlineCapacity` values inline and spills to a `pmr` arena beyond that. `PushBack`, `EmplaceBack`, `Reserve`, `Clear`, `Data`, `Size`, `IsEmpty`, `IsInline`. A full inline array without an arena spills; a copy and a move are supported.

```cpp
TArray<Tank*> Targets(&FrameArena);
TMap<FPersistentString, int> Counters;
TSet<FPersistentString> Unlocked;
TSmallArray<int, 4> Pending;
```

### TObjectPool.h — `TObjectPool<T>`

Fixed-size pool, no allocation after construction. `Acquire()` returns a free slot or `nullptr`; `Release(ptr)` returns it and returns `false` for a pointer that is not from the pool.

```cpp
TObjectPool<Bullet> Bullets(256);
if (Bullet* const Shot = Bullets.Acquire()) { ... }
Bullets.Release(Shot);
```

### TRingBuffer.h — `TRingBuffer<T, TCapacity>`

Fixed-capacity FIFO. `Push` returns `false` when full (it does not overwrite), `Pop(Out)` returns `false` when empty. `IsFull`, `IsEmpty`, `Clear`.

```cpp
TRingBuffer<GameEvent, 64> Events;
Events.Push(Event);
int Value = 0;
while (Events.Pop(Value)) { ... }
```

### TSlotMap.h — `TSlotMap<T>`, `TEntityHandle`

Items addressed by a handle (index + generation). A removed item's slot is reused, but the old handle stops resolving, so a stale handle never reaches a new item.

```cpp
TSlotMap<Tank> Tanks;
const TEntityHandle Ship = Tanks.Add(Tank{});
if (Tank* const Found = Tanks.Find(Ship)) { ... }
Tanks.Remove(Ship);      // Find(Ship) is now nullptr
```

`TEntityHandle::IsValid()` is false for a default-constructed handle.

## Enums and state

### TTemplateHelper.h

- **`TEnumArray<TEnum, TValue>`** — array indexed by an enum class with a `Count` entry. `operator[]`, `Fill`, `Size`.
- **`TEnumFlags<TEnum>`** — set of enum flags. `Set`, `Clear(Flag)`, `Toggle`, `ClearAll`, `Has(Flag)`, `HasAny(Set)`, `HasAll(Set)`, `IsEmpty`, `GetRaw`. The `TEnableEnumBitmasks` opt-in is only for the `|`, `&` operators on plain enums.
- **`TObserved<T>`** — a value with change listeners. `Get`, `Set` (listeners are called only when the value changes), `Subscribe(callback, bNotifyImmediately)` returns a `ListenerId`, `Unsubscribe(id)`.
- **`TScopeExit`** / **`MakeScopeExit`** — runs a function at scope end. **`TScopeGuard`** / **`MakeScopeGuard`** — same as `TScopeExit`, with `Dismiss()` to cancel it.
- **`TOverloaded`** — combines lambdas into one visitor for `std::visit`.
- **`TStrongType<TValue, TTag>`** — distinct types over one value (e.g. a tile index and a pixel index).
- Helpers: `EnumHasAnyFlags`, `EnumHasAllFlags`, `FNonCopyable`, `FNonCopyableNonMovable`.

```cpp
enum class DockFlag : std::uint8_t { Left, Right, Bottom };
TEnumFlags<DockFlag> Collapsed;
Collapsed.Set(DockFlag::Left);
TObserved<int> Health(10);
Health.Subscribe([](const int InNew) { ... });
```

## Strings

### TStoredString.h — `FPersistentString`, `FTransientString`

A 4-byte interned id; equality and hashing are integer operations. `View()` gives the text.

- `FPersistentString` — never freed (names from configs, ids, keys stored for the whole session).
- `FTransientString` — freed by `Clear()`/`Reset()` of its pool (per level, for strings built at runtime).

```cpp
const FPersistentString Name("tank_heavy");
std::string_view Text = Name.View();
std::unordered_map<FPersistentString, int> Hp;   // std::hash is specialized
```

Read with `std::string_view`; store the id, not the view.

## Ownership and managers

### TRefCountedHandle.h, model/RefCounted.h — `game::THandle<T>`

Shared handle for a ref-counted resource: `THandle<T>::Create<T>(args...)` creates it (or `MakeHandle<T>(args...)`), copies count the references, the last release destroys the object. Use it for shared assets (atlases, sounds) instead of raw pointers.

### TManagerSet.h — `TManagerSet<TTypeList<...>>`

Creates and destroys a list of managers in list order (destroys in reverse). `Create`, `Destroy`, `IsCreated`. `GameContext` uses it for its `GameManagers` typelist.

### TTypeList.h — `TTypeList<...>`

Compile-time list of types: `Contains<T>`, `At<Index>`, `ForEach(fn)`, `ForEachReverse(fn)`, `ApplyTo<Template>`, `Transform<Meta>`, `IndexOf`, `Concat`. Used by `TManagerSet` and `ObjectTypes`.

## Stored state and strong types

`TStrongType`, `TStoredString` and `TSlotMap` above cover stable ids; prefer them over raw `int` ids in the Model.

## Not here yet

- **TDelegate / TMulticastDelegate** (object + member callbacks) are planned in ROADMAP.md but not written yet.

## Rules

- No `std::function` in new code; use `TFunction` / `TFunctionRef`.
- Per-frame code does not allocate: use `TObjectPool`, `TRingBuffer`, `TSmallArray` with an arena, or a reserved `TArray`.
- Every new template gets unit tests in `Classes/tests/UnitTests.cpp` and a `<ClInclude>` line in `proj.win32/TanksClone.vcxproj` and `.filters`.

- `TUniquePtr<T>` / `MakeUnique<T>(...)`: sole owner of a heap object (move-only, converts to a base); `TSharedHandle<T>` / `MakeSharedValue<T>(...)` (in `model/RefCounted.h`): a value shared by copies of a handle. No `std::unique_ptr` / `std::shared_ptr` in new code.
