#pragma once

#include <string>
#include <vector>

#include "core/ObjectEnums.h"
#include "game/components/Ability.h"
#include "game/components/Weapon.h"
#include "model/ResourceId.h"

namespace tanks
{

/** Tank-specific settings of a tank config, on top of what MovableObject reads. */
struct TankDef
{
	float Armor = 0.0f;
	float ReloadTime = 0.5f;
	int MaxAmmo = 10; // negative = unlimited
	std::vector<WeaponDef> Weapons;
	std::vector<AbilityDef> Abilities; // the first one is the primary one
	std::vector<std::string> PermanentBuffs;
	EnemyType EnemyKind = EnemyType::Medium;
	int ScoreReward = 100;
	int CoinReward = 10;
	std::string DropTable; // configs/drops.json table; empty = drops nothing
	std::string SplitInto; // config of the tanks it splits into on death; empty = none
	int SplitCount = 0;
	float SplitSpacingTiles = 0.6f; // distance between the parts
	bool bFrontalShield = false;    // shots at the front do nothing
	float RearDamageScale = 1.0f;   // damage taken from shots at the back
	AiType Ai = AiType::None;
	model::ResourceId HitSound;     // a shot that hurts the tank
	model::ResourceId ArmorSound;   // a shot the armor or a shield absorbs
	model::ResourceId ExplodeSound; // invalid = silent
};

} // namespace tanks
