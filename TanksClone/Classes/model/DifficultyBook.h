#pragma once

#include <string>
#include <vector>

#include "model/AiFeatureSet.h"
#include "model/ConfigCatalog.h"

namespace game::model
{
/** One difficulty mode: multipliers for the enemies and the rewards, and whether deaths are penalized. */
struct DifficultyDef
{
	std::string Id;
	std::string NameKey;
	float EnemyHealth = 1.0f;
	float EnemyDamage = 1.0f;
	float SpawnInterval = 1.0f;
	float Coins = 1.0f;
	float Drops = 1.0f;
	bool bDeathPenalty = true;
	float BulletSpeed = 1.0f;
	float ReloadTime = 1.0f;
	float SpawnCount = 1.0f;
	float AbilityCooldown = 1.0f;
	float AiThinkTime = 1.0f;
	float AiFireRate = 1.0f;
	float AiAimTolerance = 1.0f;
	bool bFriendlyFire = false;
	float FriendlyFireShare = 1.0f;
	bool bBulletClash = false;
	AiSwitches AiFeatures;
};

/**
 * The difficulty modes (configs/balance/difficulty.json) in the order the player cycles through them.
 * An unknown id falls back to the default mode. Engine free, so it is unit-tested.
 */
class DifficultyBook
{
public:
	void Load(const IConfigNode& InNode);
	void SetModes(const std::vector<DifficultyDef>& InModes, const std::string& InDefaultId);

	const std::vector<DifficultyDef>& GetModes() const;
	const DifficultyDef& Find(const std::string& InId) const;
	std::string GetNextId(const std::string& InId) const;
	const std::string& GetDefaultId() const;
	static int ScaleCount(int InCount, float InScale);

private:
	ConfigCatalog<DifficultyDef> Modes;
	std::string DefaultId;
	DifficultyDef Neutral;
};
} // namespace game::model
