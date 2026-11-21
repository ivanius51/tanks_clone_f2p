#pragma once

#include "cocos2d.h"

namespace tanks::view::cocos
{

// Replaces the running scene with a fade (its time is "scene_fade_time" in configs/ui/widgets.json).
// Does nothing if InScene is null.
void SwitchScene(cocos2d::Scene* InScene);

} // namespace tanks::view::cocos
