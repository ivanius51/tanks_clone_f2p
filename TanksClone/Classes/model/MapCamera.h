#pragma once

#include "model/MathTypes.h"

namespace tanks::model
{

/**
 * The view onto a map: how much it is scaled and where its origin sits inside the view. A map smaller than the view
 * is centred; a bigger one follows the focus (the player) and stops at the map's edges.
 */
class MapCamera
{
public:
	// 0 = auto: the whole map fits the view; otherwise the percent of the map's own size.
	static float ComputeScale(int InScalePercent, const Vec2f& InMapSize, const Vec2f& InViewSize);

	// Where the map's origin lies inside the view (view coordinates) for a scaled map and the focus in map coordinates.
	static Vec2f ComputeOffset(const Vec2f& InFocus, float InScale, const Vec2f& InMapSize, const Vec2f& InViewSize);

private:
	static float ComputeAxisOffset(float InFocus, float InScale, float InMapSize, float InViewSize);
};

} // namespace tanks::model
