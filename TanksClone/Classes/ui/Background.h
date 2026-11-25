#pragma once

#include <string>

#include "cocos2d.h"

namespace game::view::cocos
{
cocos2d::Node* CreateBackground(unsigned InSeed, const std::string& InTileset, int InColumns, int InRows);
} // namespace game::view::cocos
