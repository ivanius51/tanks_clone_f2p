# Architecture rules

## Principles

1. OOP: separate entities, single responsibility, encapsulation (private fields, minimal public interface).
2. Behavior and numbers live in data (JSON configs), not in code: a new any entity must be added as a file or to exist file.
3. Composition over deep inheritance; dependencies are passed explicitly (injection);
4. One responsibility per class, small functions, no "god classes".
5. no magic numbers, use constexpr with the name if it cannot be placed into config (Principles:2) 
6. No hidden heap allocation in per-frame paths. Memory has a lifetime: Frame (cleared every step), Level (cleared at level start), Session (whole run); containers take the arena of their lifetime (`TArray`, `TMap`, see code-style.md "Own templates").
7. Ids and names are `TStoredString`, not owning `std::string`; the Model has no `std::function` (use `TFunction`).

## Managers
- Managers live in `GameContext` (`core/GameContext.h`): a typelist `GameManagers` sets their creation order (a manager uses only the ones listed before it) and they are destroyed in reverse. 
- A new manager is added to `GameManagers`, never as its own `GetInstance()`.
- `main` owns the context, `AppDelegate` calls `Initialize()` at launch and `Shutdown()` on `Director::EVENT_RESET`, while the engine is still alive. 
- `GameContext::Get<T>()` is the single global access point, for the View and code not yet migrated. The Model takes its dependencies explicitly (constructor parameters), never through `Get()`.

## Goal: MVVM, engine-independent core
The logic must be pluggable into another renderer: Cocos2d-x , web, Android or another engine.

- **Model** (pure C++, no `cocos2d` or other engine types): entities (tank, bullet, wall, bonus, base), stats, buffs, abilities, AI, spawners and waves, level rules, fixed-step simulation, determinism (random seed), state serialization.
- **ViewModel**: state for display (HUD values, lists, progress) and events for the View.
- **View**: the render adapter (Cocos now) and input. It only displays the ViewModel and passes user commands on.
- **Ports** between layers (interfaces): render / scene, input, audio, storage, config source, clock. Done: `ports/IAudioOutput`, `IStorage`, `IInputSource` with Cocos adapters in `platform/cocos/`; `GameContext` hands them out (`GetAudioOutput()`, `GetStorage()`, `GetInputSource()`). New code reaches audio, saved files and keys only through these ports, never through `AudioEngine`, `FileUtils::getWritablePath` or Cocos keyboard listeners.
- Dependency direction: View → ViewModel → Model. The Model knows nothing about the upper layers.
- The Model has its own math types (`model::Vec2f`, `model::RectF`, `model::Color` in `model/MathTypes.h`); engine types (`Vec2`, `Value`, `Node`) are forbidden in the Model. Conversion to engine types lives only in the View (`ui/CocosConvert.h`).
- Namespaces follow the layers: `game::model`, `game::viewmodel`, `game::ports`, `game::view::cocos` (see code-style.md).

## Current debt
Migration: ROADMAP.md, section "Engine-independent core (MVVM)".

## Tests
- The Model is tested without the engine or a window (unit tests and bots on the simulation at maximum speed).
- Level autotests run without mouse or keyboard: `TanksClone.exe --test[=mode]` (Test builds with specific test mode as parameter).
