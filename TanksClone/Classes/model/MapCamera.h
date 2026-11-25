#pragma once

#include "model/MathTypes.h"

namespace game::model
{
/**
 * The view onto a map: how much it is scaled and where its origin sits inside the view. A map smaller than the view
 * is centred; a bigger one follows the focus (the player) and stops at the map's edges.
 */
class MapCamera
{
public:
	static float ComputeScale(int InScalePercent, const Vec2f& InMapSize, const Vec2f& InViewSize);

	static Vec2f ComputeOffset(const Vec2f& InFocus, float InScale, const Vec2f& InMapSize, const Vec2f& InViewSize);

private:
	static float ComputeAxisOffset(float InFocus, float InScale, float InMapSize, float InViewSize);
};
} // namespace game::model
