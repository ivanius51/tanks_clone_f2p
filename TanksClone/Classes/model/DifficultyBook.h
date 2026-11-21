#pragma once

#include <string>
#include <vector>

#include "model/ConfigNode.h"

namespace tanks::model
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
};

/**
 * The difficulty modes (configs/difficulty.json) in the order the player cycles through them.
 * An unknown id falls back to the default mode. Engine free, so it is unit-tested.
 */
class DifficultyBook
{
public:
	// Reads "default" and a "modes" list of { "id", "name", "enemy_health", "enemy_damage", "spawn_interval", "coins", "drops", "death_penalty" }.
	void Load(const IConfigNode& InNode);
	void SetModes(const std::vector<DifficultyDef>& InModes, const std::string& InDefaultId);

	const std::vector<DifficultyDef>& GetModes() const;
	// The mode with this id, else the default one; a neutral mode when the list is empty.
	const DifficultyDef& Find(const std::string& InId) const;
	// The id after InId in the list, wrapping around.
	std::string GetNextId(const std::string& InId) const;
	const std::string& GetDefaultId() const;

private:
	std::vector<DifficultyDef> Modes;
	std::string DefaultId;
	DifficultyDef Neutral;
};

} // namespace tanks::model
