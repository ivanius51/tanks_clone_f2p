#include "model/ShotSide.h"

#include <cmath>

namespace tanks::model
{

namespace
{
// cos(45 degrees): a shot within 45 degrees of the facing axis hits the front or the back.
constexpr float GAxisCone = 0.7071f;
}

ShotSide ShotSideClassifier::Classify(const Vec2f& InFacing, const Vec2f& InShotDirection)
{
	const float Length = std::sqrt(InShotDirection.X * InShotDirection.X + InShotDirection.Y * InShotDirection.Y);
	if ( Length <= 0.0f )
	{
		return ShotSide::Side;
	}
	const float Along = (InFacing.X * InShotDirection.X + InFacing.Y * InShotDirection.Y) / Length;
	if ( Along <= -GAxisCone )
	{
		return ShotSide::Front;
	}
	return Along >= GAxisCone ? ShotSide::Back : ShotSide::Side;
}

HitFace ShotSideClassifier::ClassifyFace(const Vec2f& InFacing, const Vec2f& InShotDirection)
{
	const float Along = InFacing.X * InShotDirection.X + InFacing.Y * InShotDirection.Y;
	const float Across = InFacing.X * InShotDirection.Y - InFacing.Y * InShotDirection.X;
	if ( std::fabs(Along) >= std::fabs(Across) )
	{
		return Along < 0.0f ? HitFace::Front : HitFace::Back;
	}
	return Across < 0.0f ? HitFace::Left : HitFace::Right;
}

} // namespace tanks::model
