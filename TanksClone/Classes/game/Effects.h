#pragma once

#include "cocos2d.h"

namespace tanks
{
namespace effects
{

// Short animation that removes itself when done. Does nothing if InParent is null.
// A big explosion is for tanks, a small one for hits and walls.
void Explosion(cocos2d::Node* InParent, const cocos2d::Vec2& InPosition, bool bInBig);

// Brief flash where a tank is about to appear.
void SpawnFlash(cocos2d::Node* InParent, const cocos2d::Vec2& InPosition);

} // namespace effects
} // namespace tanks
