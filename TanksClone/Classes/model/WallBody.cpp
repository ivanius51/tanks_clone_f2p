#include "model/WallBody.h"

#include <algorithm>
#include <cmath>

namespace game::model
{
namespace
{
constexpr float QuartersPerTile = 4.0f;
}

WallBody::WallBody(const int InCellsPerSide, const int InCellHealth)
	: Mask(InCellsPerSide, InCellHealth)
{
}

float WallBody::GetCellSize(const RectF& InBox) const
{
	return InBox.Size.X / static_cast<float>(Mask.GetCellsPerSide());
}

bool WallBody::Blocks(const RectF& InBox, const RectF& InRect) const
{
	const float Cell = GetCellSize(InBox);
	if ( Cell <= 0.0f )
	{
		return false;
	}
	return Mask.Overlaps((InRect.GetMinX() - InBox.GetMinX()) / Cell, (InRect.GetMinY() - InBox.GetMinY()) / Cell,
		(InRect.GetMaxX() - InBox.GetMinX()) / Cell, (InRect.GetMaxY() - InBox.GetMinY()) / Cell);
}

bool WallBody::SweepHit(const RectF& InBox, const Vec2f& InFrom, const Vec2f& InTo, const Vec2f& InHalf, float& OutFraction) const
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

int WallBody::Chip(const RectF& InBox, const RectF& InArea, const HitSide InSide, const int InDepthQuarters, const int InDamage)
{
	if ( !InArea.Intersects(InBox) )
	{
		return 0;
	}
	const float Cell = GetCellSize(InBox);
	const bool bVertical = InSide == HitSide::Bottom || InSide == HitSide::Top;
	const float Low = bVertical ? (InArea.GetMinX() - InBox.GetMinX()) / Cell : (InArea.GetMinY() - InBox.GetMinY()) / Cell;
	const float High = bVertical ? (InArea.GetMaxX() - InBox.GetMinX()) / Cell : (InArea.GetMaxY() - InBox.GetMinY()) / Cell;
	const int Depth = std::max(1, static_cast<int>(std::ceil(static_cast<float>(InDepthQuarters * Mask.GetCellsPerSide()) / QuartersPerTile)));
	return Mask.Chip(InSide, Low, High, Depth, InDamage);
}
} // namespace game::model
