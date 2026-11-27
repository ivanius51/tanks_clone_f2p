#include "model/BulletClash.h"

#include <algorithm>
#include <cmath>

namespace game::model
{

ClashResult Clash(const int InHealthA, const int InHealthB)
{
	const int Lost = std::max(0, std::min(InHealthA, InHealthB));
	return { std::max(0, InHealthA - Lost), std::max(0, InHealthB - Lost) };
}

int FriendlyDamage(const int InDamage, const float InShare)
{
	if ( InDamage <= 0 || InShare <= 0.0f )
	{
		return 0;
	}
	return std::max(1, static_cast<int>(std::lround(InDamage * InShare)));
}

ProjectileHitOutcome ResolveProjectileHit(const ProjectileHitInput& InHit)
{
	ProjectileHitOutcome Outcome;
	Outcome.BouncesLeft = InHit.BouncesLeft;
	if ( InHit.Target == ProjectileTarget::Wall )
	{
		if ( InHit.BouncesLeft > 0 )
		{
			Outcome.Action = ProjectileAction::Bounce;
			Outcome.BouncesLeft = InHit.BouncesLeft - 1;
			Outcome.bShellSpent = false;
			return Outcome;
		}
		Outcome.Action = ProjectileAction::ChipWall;
		Outcome.Damage = InHit.Damage;
		return Outcome;
	}
	Outcome.Action = ProjectileAction::DamageTarget;
	Outcome.Damage = InHit.bSameTeam ? FriendlyDamage(InHit.Damage, InHit.FriendlyShare) : InHit.Damage;
	return Outcome;
}

} // namespace game::model
