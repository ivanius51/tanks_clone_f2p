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
	float BulletSpeed = 1.0f;
	float ReloadTime = 1.0f;
	float SpawnCount = 1.0f;
	float AbilityCooldown = 1.0f; // enemy ability cooldowns
	float AiThinkTime = 1.0f;     // time between AI decisions (reaction)
	float AiFireRate = 1.0f;      // how often the AI fires at random
	float AiAimTolerance = 1.0f;  // how far off line the AI still fires at its target
	bool bFriendlyFire = false;   // enemy shots hurt other enemies
};

/**
 * The difficulty modes (configs/difficulty.json) in the order the player cycles through them.
 * An unknown id falls back to the default mode. Engine free, so it is unit-tested.
 */
class DifficultyBook
{
public:
	// Reads "default" and a "modes" list of { "id", "name", "enemy_health", "enemy_damage", "spawn_interval", "coins", "drops",
	// "bullet_speed", "reload_time", "spawn_count", "ability_cooldown", "ai_think_time", "ai_fire_rate",
	// "ai_aim_tolerance", "friendly_fire", "death_penalty" }.
	void Load(const IConfigNode& InNode);
	void SetModes(const std::vector<DifficultyDef>& InModes, const std::string& InDefaultId);

	const std::vector<DifficultyDef>& GetModes() const;
	// The mode with this id, else the default one; a neutral mode when the list is empty.
	const DifficultyDef& Find(const std::string& InId) const;
	// The id after InId in the list, wrapping around.
	std::string GetNextId(const std::string& InId) const;
	const std::string& GetDefaultId() const;
	// A wave entry's enemy count scaled by InScale, rounded; never below one.
	static int ScaleCount(int InCount, float InScale);

private:
	std::vector<DifficultyDef> Modes;
	std::string DefaultId;
	DifficultyDef Neutral;
};

} // namespace tanks::model
