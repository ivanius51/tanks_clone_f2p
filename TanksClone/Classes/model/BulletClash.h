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
} // namespace game::model
