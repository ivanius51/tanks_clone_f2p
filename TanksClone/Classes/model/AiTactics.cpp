#include "model/AiTactics.h"

#include <algorithm>
#include <cmath>

namespace tanks::model
{

Vec2f AiTactics::LeadPoint(const Vec2f& InShooter, const Vec2f& InTarget, const Vec2f& InTargetVelocity, const float InBulletSpeed)
{
	if ( InBulletSpeed <= 0.0f )
	{
		return InTarget;
	}
	Vec2f Aim = InTarget;
	for ( int Step = 0; Step < 3; ++Step )
	{
		Aim = InTarget + InTargetVelocity * (InShooter.DistanceTo(Aim) / InBulletSpeed);
	}
	return Aim;
}

bool AiTactics::IsThreat(const Vec2f& InPosition, const float InRadius, const Vec2f& InBulletPosition, const Vec2f& InBulletVelocity, const float InLookahead)
{
	const float SpeedSquared = InBulletVelocity.LengthSquared();
	if ( SpeedSquared <= 0.0f )
	{
		return false;
	}
	const Vec2f ToTarget = InPosition - InBulletPosition;
	const float Time = ToTarget.Dot(InBulletVelocity) / SpeedSquared;
	if ( Time < 0.0f || Time > InLookahead )
	{
		return false;
	}
	return (InBulletPosition + InBulletVelocity * Time).DistanceTo(InPosition) < InRadius;
}

Vec2f AiTactics::DodgeDirection(const Vec2f& InPosition, const Vec2f& InBulletPosition, const Vec2f& InBulletVelocity)
{
	const bool bBulletVertical = std::fabs(InBulletVelocity.Y) >= std::fabs(InBulletVelocity.X);
	const Vec2f Offset = InPosition - InBulletPosition;
	if ( bBulletVertical )
	{
		return { Offset.X >= 0.0f ? 1.0f : -1.0f, 0.0f };
	}
	return { 0.0f, Offset.Y >= 0.0f ? 1.0f : -1.0f };
}

bool AiTactics::ShouldRetreat(const float InHealthShare, const float InRetreatBelow)
{
	return InHealthShare <= InRetreatBelow;
}

bool AiTactics::IsClosest(const float InOwn, const std::vector<float>& InOthers)
{
	return std::none_of(InOthers.begin(), InOthers.end(), [InOwn](const float InOther) { return InOther < InOwn; });
}

bool AiTactics::IsExposed(const Vec2f& InPosition, const Vec2f& InShooter, const float InTolerance)
{
	return std::fabs(InPosition.X - InShooter.X) < InTolerance || std::fabs(InPosition.Y - InShooter.Y) < InTolerance;
}

Vec2f AiTactics::SidestepDirection(const Vec2f& InPosition, const Vec2f& InShooter)
{
	const Vec2f Offset = InPosition - InShooter;
	if ( std::fabs(Offset.X) < std::fabs(Offset.Y) )
	{
		return { Offset.X < 0.0f ? -1.0f : 1.0f, 0.0f };
	}
	return { 0.0f, Offset.Y < 0.0f ? -1.0f : 1.0f };
}

} // namespace tanks::model
