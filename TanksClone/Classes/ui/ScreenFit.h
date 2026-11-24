#pragma once

#include <functional>

#include "cocos2d.h"

#include "model/ScreenLayout.h"

namespace tanks::view::cocos
{
constexpr const char* ScreenLayoutChangedEvent = "tanks_screen_layout_changed";

model::ScreenLayout GetScreenLayout();
void ApplyScreenFit();
void WatchWindowResizes();

void OnScreenLayoutChanged(cocos2d::Node* InOwner, const std::function<void(const model::ScreenLayout&)>& InHandler);
void CenterOnScreen(cocos2d::Scene* InScene);
void PlaceScene(cocos2d::Scene* InScene, const cocos2d::Vec2& InOffset);
void CoverScreen(cocos2d::Node* InLayer);
} // namespace tanks::view::cocos
