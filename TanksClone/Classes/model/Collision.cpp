#include "model/Collision.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace game::model::collision
{
namespace
{
constexpr float ParallelEpsilon = 1e-6f;
constexpr float HalfTurnDegrees = 180.0f;
constexpr float QuarterTurnLow = 45.0f;
constexpr float QuarterTurnHigh = 135.0f;
}

RectF MakeBodyRect(const Vec2f& InCenter, const Vec2f& InSize, const Vec2f& InScale, const float InRotationDegrees, const bool bInVehicle)
{
	float Width = InSize.X * InScale.X;
	float Height = InSize.Y * InScale.Y;
	if ( bInVehicle )
	{
		Width = Height = std::max(Width, Height);
	}
	const float Angle = std::fabs(std::fmod(InRotationDegrees, HalfTurnDegrees));
	if ( Angle > QuarterTurnLow && Angle < QuarterTurnHigh )
	{
		std::swap(Width, Height);
	}
	return RectF(InCenter.X - Width / 2.0f, InCenter.Y - Height / 2.0f, Width, Height);
}

float OverlapArea(const RectF& InFirst, const RectF& InSecond)
{
	const float Width = std::min(InFirst.GetMaxX(), InSecond.GetMaxX()) - std::max(InFirst.GetMinX(), InSecond.GetMinX());
	const float Height = std::min(InFirst.GetMaxY(), InSecond.GetMaxY()) - std::max(InFirst.GetMinY(), InSecond.GetMinY());
	return Width > 0.0f && Height > 0.0f ? Width * Height : 0.0f;
}

bool IsInside(const RectF& InBounds, const RectF& InRect)
{
	return InRect.GetMinX() >= InBounds.GetMinX() && InRect.GetMaxX() <= InBounds.GetMaxX()
		&& InRect.GetMinY() >= InBounds.GetMinY() && InRect.GetMaxY() <= InBounds.GetMaxY();
}

bool SweepBox(const RectF& InBox, const Vec2f& InFrom, const Vec2f& InTo, const Vec2f& InHalf, float& OutFraction)
{
	const std::array<float, 2> Delta = { InTo.X - InFrom.X, InTo.Y - InFrom.Y };
	const std::array<float, 2> From = { InFrom.X, InFrom.Y };
	const std::array<float, 2> Low = { InBox.GetMinX() - InHalf.X, InBox.GetMinY() - InHalf.Y };
	const std::array<float, 2> High = { InBox.GetMaxX() + InHalf.X, InBox.GetMaxY() + InHalf.Y };

	float Enter = 0.0f;
	float Exit = 1.0f;
	for ( int Axis = 0; Axis < 2; ++Axis )
	{
		if ( std::fabs(Delta[Axis]) < ParallelEpsilon )
		{
			if ( From[Axis] < Low[Axis] || From[Axis] > High[Axis] )
			{
				return false;
			}
			continue;
		}
		float T1 = (Low[Axis] - From[Axis]) / Delta[Axis];
		float T2 = (High[Axis] - From[Axis]) / Delta[Axis];
		if ( T1 > T2 )
		{
			std::swap(T1, T2);
		}
		Enter = std::max(Enter, T1);
		Exit = std::min(Exit, T2);
		if ( Enter > Exit )
		{
			return false;
		}
	}
	OutFraction = Enter;
	return true;
}
} // namespace game::model::collision
