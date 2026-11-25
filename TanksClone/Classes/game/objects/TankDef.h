#pragma once

#include <string>
#include <vector>

#include "core/ObjectEnums.h"
#include "game/components/Ability.h"
#include "game/components/Weapon.h"
#include "model/AiFeatureSet.h"
#include "model/ResourceId.h"

namespace game
{
/** One boss phase: what the tank gains once its health share drops to HealthBelow. */
struct TankPhaseDef
{
	float HealthBelow = 0.5f;
	std::vector<AbilityDef> Abilities;
	std::vector<std::string> Buffs;
	std::string SummonConfig;
	int SummonCount = 0;
	float SummonSpacingTiles = 1.5f;
	model::ResourceId Sound;
};

/** Tank-specific settings of a tank config, on top of what MovableObject reads. */
struct TankDef
{
	float Armor = 0.0f;
	float ReloadTime = 0.5f;
	int MaxAmmo = 10;
	std::vector<WeaponDef> Weapons;
	std::vector<AbilityDef> Abilities;
	std::vector<std::string> PermanentBuffs;
	EEnemyType EnemyKind = EEnemyType::Medium;
	int ScoreReward = 100;
	int CoinReward = 10;
	std::string DropTable;
	std::string SplitInto;
	int SplitCount = 0;
	float SplitSpacingTiles = 0.6f;
	bool bFrontalShield = false;
	float RearDamageScale = 1.0f;
	std::vector<TankPhaseDef> Phases;
	EAiType Ai = EAiType::None;
	model::AiSwitches AiFeatures;
	model::ResourceId HitSound;
	model::ResourceId ArmorSound;
	model::ResourceId ExplodeSound;
};
} // namespace game
