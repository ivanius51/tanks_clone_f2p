#pragma once

#include "model/RenderLayer.h"

namespace game::view::cocos
{
/** The engine z order of a named layer, read once from the render layers config; the offset orders nodes inside one layer. */
int GetZOrder(model::RenderLayer InLayer, int InOffset = 0);
} // namespace game::view::cocos
