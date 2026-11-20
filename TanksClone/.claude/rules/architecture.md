# Architecture rules

## Principles

- OOP: separate entities, single responsibility, encapsulation (private fields, minimal public interface).
- Behavior and numbers live in data (JSON configs), not in code: a new enemy, buff, level or upgrade is added as a file.
- Composition over deep inheritance; dependencies are passed explicitly (injection); don't add new singletons.
- One responsibility per class, small functions, no "god classes".

## Goal: MVVM, engine-independent core

The logic must be pluggable into another renderer: Cocos2d-x now, later web, Android or another engine.

- **Model** (pure C++, no `cocos2d` or other engine types): entities (tank, bullet, wall, bonus, base), stats, buffs, abilities, AI, spawners and waves, level rules, fixed-step simulation, determinism (random seed), state serialization.
- **ViewModel**: state for display (HUD values, lists, progress) and events for the View.
- **View**: the render adapter (Cocos now) and input. It only displays the ViewModel and passes user commands on.
- **Ports** between layers (interfaces): render / scene, input, audio, storage, config source, clock.
- Dependency direction: View → ViewModel → Model. The Model knows nothing about the upper layers.
- The Model has its own math types (`model::Vec2f`, `model::RectF`, `model::Color` in `model/MathTypes.h`); engine types (`Vec2`, `Value`, `Node`) are forbidden in the Model. Conversion to engine types lives only in the View (`ui/CocosConvert.h`).
- Namespaces follow the layers: `tanks::model`, `tanks::viewmodel`, `tanks::ports`, `tanks::view::cocos` (see code-style.md).

## Current debt

`GameObject` holds a `cocos2d::Node`, configs are read through `cocos2d::Value`, updates run through the engine scheduler, and scenes contain game rules. Migration: ROADMAP.md, section "Engine-independent core (MVVM)".

## Tests

- The Model is tested without the engine or a window (unit tests and bots on the simulation at maximum speed).
- Level autotests run without mouse or keyboard: `TanksClone.exe --test` (Debug and Test builds).
