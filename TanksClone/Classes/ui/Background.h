#pragma once

#include <string>

#include "cocos2d.h"

namespace tanks::view::cocos
{
cocos2d::Node* CreateBackground(unsigned InSeed, const std::string& InTileset, int InColumns, int InRows);
} // namespace tanks::view::cocos
