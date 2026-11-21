#pragma once

#include <vector>

#include "model/MathTypes.h"

namespace tanks::model
{

/**
 * The geometry behind an enemy's smarter choices: where to aim at a moving target,
 * whether a bullet is about to hit, which way to step aside, when to fall back.
 */
class AiTactics
{
public:
	// Where a target moving at InTargetVelocity will be when a shot at InBulletSpeed reaches it.
	static Vec2f LeadPoint(const Vec2f& InShooter, const Vec2f& InTarget, const Vec2f& InTargetVelocity, float InBulletSpeed);
	// True if a bullet passes within InRadius of InPosition during the next InLookahead seconds.
	static bool IsThreat(const Vec2f& InPosition, float InRadius, const Vec2f& InBulletPosition, const Vec2f& InBulletVelocity, float InLookahead);
	// The axis direction across the bullet's flight that takes InPosition out of its way fastest.
	static Vec2f DodgeDirection(const Vec2f& InPosition, const Vec2f& InBulletPosition, const Vec2f& InBulletVelocity);
	static bool ShouldRetreat(float InHealthShare, float InRetreatBelow);
	// True if no one of InOthers is nearer than InOwn (ties go to this one).
	static bool IsClosest(float InOwn, const std::vector<float>& InOthers);
	// True if InPosition shares a row or column with InShooter (within InTolerance), so a shot can reach it.
	static bool IsExposed(const Vec2f& InPosition, const Vec2f& InShooter, float InTolerance);
	// The axis direction that leaves the shooter's row or column, away from the shooter's side.
	static Vec2f SidestepDirection(const Vec2f& InPosition, const Vec2f& InShooter);
};

} // namespace tanks::model
