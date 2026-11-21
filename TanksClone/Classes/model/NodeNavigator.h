#pragma once

#include <vector>

#include "model/MathTypes.h"

namespace tanks::model
{

/** Moves a keyboard or gamepad focus between points laid out freely on a screen: to the nearest one in the pressed direction. */
class NodeNavigator
{
public:
	// InSelectable may be empty (all points can be chosen). Returns InFrom when nothing lies that way.
	static int FindNext(const std::vector<Vec2f>& InPoints, const std::vector<bool>& InSelectable, int InFrom, const Vec2f& InDirection);
};

} // namespace tanks::model
