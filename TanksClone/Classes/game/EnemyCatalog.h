#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "core/ObjectEnums.h"

namespace tanks
{

// One enemy found in configs/enemies/. The file name is the enemy's id.
struct EnemyDef
{
	std::string Id;        // "Sniper" (from Sniper.json)
	std::string ConfigId;  // "configs/enemies/Sniper.json"
	std::string NameKey;   // string id for the editor; the id itself if the config has no "name"
	EnemyType Kind = EnemyType::Medium; // its class
	std::string Atlas;     // "textures/enemies/Sniper.plist": loaded only for the levels that can send this enemy
};

// All enemies of the game. A new enemy is just a new JSON file in configs/enemies/ that
// combines stats ("health", "speed"...), "abilities", "buffs", the "ai" and an "enemy_type"
// (class): no code changes are needed.
class EnemyCatalog
{
public:
	EnemyCatalog() = default;
	~EnemyCatalog() = default;
	EnemyCatalog(const EnemyCatalog&) = delete;
	EnemyCatalog& operator=(const EnemyCatalog&) = delete;

	// Call after the configs are loaded.
	void Init();

	const std::vector<EnemyDef>& GetAll() const;
	// nullptr if there is no such enemy.
	const EnemyDef* Find(std::string_view InId) const;
	std::vector<const EnemyDef*> GetByType(EnemyType InType) const;

private:
	std::vector<EnemyDef> Enemies;
};

} // namespace tanks
