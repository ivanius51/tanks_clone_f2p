#include "model/GridBody.h"

#include <algorithm>
#include <cmath>

namespace game::model
{
namespace
{
constexpr float QuartersPerTile = 4.0f;
}

GridBody::GridBody(const int InCellsPerSide, const int InCellHealth)
	: Mask(InCellsPerSide, InCellHealth)
{
}

float GridBody::GetCellSize(const RectF& InBox) const
{
	return InBox.Size.X / static_cast<float>(Mask.GetCellsPerSide());
}

bool GridBody::Blocks(const RectF& InBox, const RectF& InRect) const
{
	const float Cell = GetCellSize(InBox);
	if ( Cell <= 0.0f )
	{
		return false;
	}
	return Mask.Overlaps((InRect.GetMinX() - InBox.GetMinX()) / Cell, (InRect.GetMinY() - InBox.GetMinY()) / Cell,
		(InRect.GetMaxX() - InBox.GetMinX()) / Cell, (InRect.GetMaxY() - InBox.GetMinY()) / Cell);
}

bool GridBody::SweepHit(const RectF& InBox, const Vec2f& InFrom, const Vec2f& InTo, const Vec2f& InHalf, float& OutFraction) const
{
	const float Cell = GetCellSize(InBox);
	if ( Cell <= 0.0f )
	{
		return false;
	}
	const Vec2f From = (InFrom - InBox.Origin) / Cell;
	const Vec2f To = (InTo - InBox.Origin) / Cell;
	return Mask.Sweep(From.X, From.Y, To.X, To.Y, std::max(InHalf.X, InHalf.Y) / Cell, OutFraction);
}

namespace
{
Vec2f GetHeading(const HitSide InSide)
{
	switch ( InSide )
	{
	case HitSide::Bottom:
		return Vec2f(0.0f, 1.0f);
	case HitSide::Top:
		return Vec2f(0.0f, -1.0f);
	case HitSide::Left:
		return Vec2f(1.0f, 0.0f);
	default:
		return Vec2f(-1.0f, 0.0f);
	}
}
}

int GridBody::Chip(const RectF& InBox, const RectF& InArea, const HitSide InSide, const int InDepthQuarters, const int InDamage)
{
	GridCarveResult Result;
	Chip(InBox, InArea, InSide, InDepthQuarters, InDamage, Result);
	return Result.Removed;
}

void GridBody::Chip(const RectF& InBox, const RectF& InArea, const HitSide InSide, const int InDepthQuarters, const int InDamage, GridCarveResult& OutResult)
{
	OutResult.Heading = GetHeading(InSide);
	if ( !InArea.Intersects(InBox) )
	{
		return;
	}
	const float Cell = GetCellSize(InBox);
	const bool bVertical = InSide == HitSide::Bottom || InSide == HitSide::Top;
	const float Low = bVertical ? (InArea.GetMinX() - InBox.GetMinX()) / Cell : (InArea.GetMinY() - InBox.GetMinY()) / Cell;
	const float High = bVertical ? (InArea.GetMaxX() - InBox.GetMinX()) / Cell : (InArea.GetMaxY() - InBox.GetMinY()) / Cell;
	const int Depth = std::max(1, static_cast<int>(std::ceil(static_cast<float>(InDepthQuarters * Mask.GetCellsPerSide()) / QuartersPerTile)));
	OutResult.Removed = Mask.Chip(InSide, Low, High, Depth, InDamage, &OutResult.BrokenCells);
}
} // namespace game::model
