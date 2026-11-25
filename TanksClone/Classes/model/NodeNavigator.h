#pragma once

#include <vector>

#include "model/MathTypes.h"

namespace game::model
{
/** Moves a keyboard or gamepad focus between points laid out freely on a screen: to the nearest one in the pressed direction. */
class NodeNavigator
{
public:
	static int FindNext(const std::vector<Vec2f>& InPoints, const std::vector<bool>& InSelectable, int InFrom, const Vec2f& InDirection);
};
} // namespace game::model
