#pragma once


#include "model/MathTypes.h"
#include "templates/TContainers.h"

namespace game::model
{
/** Moves a keyboard or gamepad focus between points laid out freely on a screen: to the nearest one in the pressed direction. */
class NodeNavigator
{
public:
	static int FindNext(const TArray<Vec2f>& InPoints, const TArray<uint8_t>& InSelectable, int InFrom, const Vec2f& InDirection);
};
} // namespace game::model
