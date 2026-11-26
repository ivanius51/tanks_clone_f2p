#pragma once

#include "model/MathTypes.h"
#include "model/CellGrid.h"

namespace game::model
{
/**
 * The collision shape of one wall tile: its CellGrid placed in a box. Answers overlap, sweep and chip questions in
 * world units; the box is passed in because the owner moves. No engine types.
 */
class GridBody
{
public:
	GridBody(int InCellsPerSide, int InCellHealth);

	const CellGrid& GetMask() const { return Mask; }
	CellGrid& GetMask() { return Mask; }

	float GetCellSize(const RectF& InBox) const;
	bool Blocks(const RectF& InBox, const RectF& InRect) const;
	bool SweepHit(const RectF& InBox, const Vec2f& InFrom, const Vec2f& InTo, const Vec2f& InHalf, float& OutFraction) const;
	// The depth is counted in quarter tiles, so a wall of any cell resolution loses the same physical depth.
	int Chip(const RectF& InBox, const RectF& InArea, HitSide InSide, int InDepthQuarters, int InDamage);
	// Also reports which cells fell and the direction the shell travelled.
	void Chip(const RectF& InBox, const RectF& InArea, HitSide InSide, int InDepthQuarters, int InDamage, GridCarveResult& OutResult);

private:
	CellGrid Mask;
};
} // namespace game::model
