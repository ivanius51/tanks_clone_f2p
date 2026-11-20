#pragma once

#include <string>

#include "cocos2d.h"

namespace tanks::view::cocos
{

// The ground of the playfield: ground tiles of the tileset (its atlas must be loaded) and a frame around it.
// The same seed always gives the same picture. Without the tileset's frames the floor is a plain checkerboard.
cocos2d::Node* CreateBackground(unsigned InSeed, const std::string& InTileset);

} // namespace tanks::view::cocos
