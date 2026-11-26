#pragma once


#include "model/MathTypes.h"
#include "templates/TContainers.h"

namespace game::model
{
/**
 * The geometry behind an enemy's smarter choices: where to aim at a moving target,
 * whether a bullet is about to hit, which way to step aside, when to fall back.
 */
class AiTactics
{
public:
	static Vec2f LeadPoint(const Vec2f& InShooter, const Vec2f& InTarget, const Vec2f& InTargetVelocity, float InBulletSpeed);
	static bool IsThreat(const Vec2f& InPosition, float InRadius, const Vec2f& InBulletPosition, const Vec2f& InBulletVelocity, float InLookahead);
	static Vec2f DodgeDirection(const Vec2f& InPosition, const Vec2f& InBulletPosition, const Vec2f& InBulletVelocity);
	static bool ShouldRetreat(float InHealthShare, float InRetreatBelow);
	static bool IsClosest(float InOwn, const TArray<float>& InOthers);
	static bool IsExposed(const Vec2f& InPosition, const Vec2f& InShooter, float InTolerance);
	static Vec2f SidestepDirection(const Vec2f& InPosition, const Vec2f& InShooter);
};
} // namespace game::model
