#pragma once

#include <string>

#include "cocos2d.h"

#include "model/LevelLegend.h"
#include "templates/TContainers.h"

namespace game::view::cocos
{
cocos2d::Node* CreateBackground(unsigned InSeed, const std::string& InTileset, int InColumns, int InRows, const TArray<std::string>& InGround, const model::LevelLegend& InLegend, bool bInMergedFloor);
} // namespace game::view::cocos
