#pragma once

namespace game::model
{
/** Health left on two bullets after they meet: each loses the other's health, so the weaker one breaks. */
struct ClashResult
{
	int LeftA = 0;
	int LeftB = 0;
};

ClashResult Clash(int InHealthA, int InHealthB);

/** Damage of a shot that hits its own team: scaled by the difficulty's friendly_fire_share, at least 1 while the share is above 0. */
int FriendlyDamage(int InDamage, float InShare);

/** What a shell struck. */
enum class ProjectileTarget
{
	Wall,
	Tank,
	Other
};

/** What the shell does about it. */
enum class ProjectileAction
{
	Bounce,
	ChipWall,
	DamageTarget
};

struct ProjectileHitInput
{
	ProjectileTarget Target = ProjectileTarget::Other;
	int BouncesLeft = 0;
	int Damage = 0;
	bool bSameTeam = false;
	float FriendlyShare = 0.0f;
};

struct ProjectileHitOutcome
{
	ProjectileAction Action = ProjectileAction::DamageTarget;
	int Damage = 0;
	int BouncesLeft = 0;
	bool bShellSpent = true;
};

/** Decides the outcome of a shell hit: a wall bounces it while bounces remain, otherwise it is spent; own team takes the friendly share. */
ProjectileHitOutcome ResolveProjectileHit(const ProjectileHitInput& InHit);
} // namespace game::model
