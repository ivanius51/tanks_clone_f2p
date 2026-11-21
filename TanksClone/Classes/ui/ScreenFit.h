#pragma once

#include <functional>

#include "cocos2d.h"

#include "model/ScreenLayout.h"

namespace tanks::view::cocos
{

// Sent after the visible area changed: the window was resized or switched between windowed and borderless.
constexpr const char* ScreenLayoutChangedEvent = "tanks_screen_layout_changed";

// The layout of the design area in the game window as it is now.
model::ScreenLayout GetScreenLayout();
// Makes the engine draw the whole window (no black bars) with the design area centred, then sends
// ScreenLayoutChangedEvent. Called at start and after every window resize.
void ApplyScreenFit();
// ApplyScreenFit after every resize of the engine's window.
void WatchWindowResizes();

// Calls InHandler now and after every layout change while InOwner exists (it runs only while InOwner is on screen).
void OnScreenLayoutChanged(cocos2d::Node* InOwner, const std::function<void(const model::ScreenLayout&)>& InHandler);
// Keeps the design area of a scene in the middle of the window: its children keep design coordinates.
void CenterOnScreen(cocos2d::Scene* InScene);
// Shifts the scene by the design offset. Its own default camera is its child, so the camera is moved back to
// keep looking at the middle of the window; otherwise the shift cancels out once the scene runs on its own.
void PlaceScene(cocos2d::Scene* InScene, const cocos2d::Vec2& InOffset);
// Stretches InLayer over the whole visible area (in its parent's design coordinates).
void CoverScreen(cocos2d::Node* InLayer);

} // namespace tanks::view::cocos
