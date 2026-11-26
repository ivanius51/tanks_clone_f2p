#pragma once

#include "model/MathTypes.h"

namespace game::model::collision
{
/** The box a body takes: its size is the sprite size times scale, squared up for a vehicle, with the sides swapped at a quarter turn. */
RectF MakeBodyRect(const Vec2f& InCenter, const Vec2f& InSize, const Vec2f& InScale, float InRotationDegrees, bool bInVehicle);

/** Area of the overlap of two boxes; 0 when they only touch or are apart. */
float OverlapArea(const RectF& InFirst, const RectF& InSecond);

/** Whether the box lies fully inside the bounds. */
bool IsInside(const RectF& InBounds, const RectF& InRect);

/** Sweeps a box with the given half size from one point to another against a box; true with the entry fraction (0-1) on a hit. */
bool SweepBox(const RectF& InBox, const Vec2f& InFrom, const Vec2f& InTo, const Vec2f& InHalf, float& OutFraction);
} // namespace game::model::collision
