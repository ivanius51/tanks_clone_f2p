#pragma once

#include "model/MathTypes.h"
#include "model/WallMask.h"

namespace game::model
{
/**
 * The collision shape of one wall tile: its WallMask placed in a box. Answers overlap, sweep and chip questions in
 * world units; the box is passed in because the owner moves. No engine types.
 */
class WallBody
{
public:
	WallBody(int InCellsPerSide, int InCellHealth);

	const WallMask& GetMask() const { return Mask; }
	WallMask& GetMask() { return Mask; }

	float GetCellSize(const RectF& InBox) const;
	bool Blocks(const RectF& InBox, const RectF& InRect) const;
	bool SweepHit(const RectF& InBox, const Vec2f& InFrom, const Vec2f& InTo, const Vec2f& InHalf, float& OutFraction) const;
	// The depth is counted in quarter tiles, so a wall of any cell resolution loses the same physical depth.
	int Chip(const RectF& InBox, const RectF& InArea, HitSide InSide, int InDepthQuarters, int InDamage);

private:
	WallMask Mask;
};
} // namespace game::model
