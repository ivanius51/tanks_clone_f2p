#pragma once

#include "cocos2d.h"

#include "model/Gradient.h"

namespace game::view::cocos
{

// Fills a rectangle centred on the origin (InHalf is half its size) with rounded corners and a gradient, as thin strips along the gradient.
void FillGradientRect(cocos2d::DrawNode* InNode, const cocos2d::Size& InHalf, float InRadius, const model::Gradient& InGradient);

} // namespace game::view::cocos
