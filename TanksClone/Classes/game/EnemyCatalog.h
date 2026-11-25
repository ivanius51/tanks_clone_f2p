#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "core/ObjectEnums.h"
#include "model/ConfigCatalog.h"

namespace game
{
/** One enemy found in configs/enemies/. The file name is the enemy's id. */
struct EnemyDef
{
	std::string Id;
	std::string ConfigId;
	std::string NameKey;
	EnemyType Kind = EnemyType::Medium;
	std::string Atlas;
};

/**
 * All enemies of the game. A new enemy is just a new JSON file in configs/enemies/ that
 * combines stats ("health", "speed"...), "abilities", "buffs", the "ai" and an "enemy_type"
 * (class): no code changes are needed.
 */
class EnemyCatalog
{
public:
	EnemyCatalog() = default;
	~EnemyCatalog() = default;
	EnemyCatalog(const EnemyCatalog&) = delete;
	EnemyCatalog& operator=(const EnemyCatalog&) = delete;

	void Init();

	const std::vector<EnemyDef>& GetAll() const;
	const EnemyDef* Find(std::string_view InId) const;
	std::vector<const EnemyDef*> GetByType(EnemyType InType) const;

private:
	model::ConfigCatalog<EnemyDef> Enemies;
};
} // namespace game
