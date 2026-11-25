#include "model/MapCamera.h"

#include <algorithm>

namespace game::model
{

float MapCamera::ComputeScale(const int InScalePercent, const Vec2f& InMapSize, const Vec2f& InViewSize)
{
	if ( InScalePercent > 0 )
	{
		return static_cast<float>(InScalePercent) / 100.0f;
	}
	if ( InMapSize.X <= 0.0f || InMapSize.Y <= 0.0f )
	{
		return 1.0f;
	}
	return std::min(InViewSize.X / InMapSize.X, InViewSize.Y / InMapSize.Y);
}

Vec2f MapCamera::ComputeOffset(const Vec2f& InFocus, const float InScale, const Vec2f& InMapSize, const Vec2f& InViewSize)
{
	return { ComputeAxisOffset(InFocus.X, InScale, InMapSize.X, InViewSize.X), ComputeAxisOffset(InFocus.Y, InScale, InMapSize.Y, InViewSize.Y) };
}

float MapCamera::ComputeAxisOffset(const float InFocus, const float InScale, const float InMapSize, const float InViewSize)
{
	const float Scaled = InMapSize * InScale;
	if ( Scaled <= InViewSize )
	{
		return (InViewSize - Scaled) * 0.5f;
	}
	return std::clamp(InViewSize * 0.5f - InFocus * InScale, InViewSize - Scaled, 0.0f);
}

} // namespace game::model
