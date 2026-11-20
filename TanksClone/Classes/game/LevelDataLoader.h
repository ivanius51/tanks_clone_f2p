#pragma once

#include <string>
#include <vector>
#include "game/LevelLoader.h"
#include "model/ConfigNode.h"

namespace tanks
{

/** The only place that knows the layout of a level config; LevelLoader gets a plain LevelData. */
class LevelDataLoader
{
public:
	// False when "rows" is missing. Enemy ids are not checked against the catalog here.
	// Without "waves" the level gets one wave from the default spawner (the simple form).
	static bool ReadLevel(const model::IConfigNode& InNode, LevelData& OutLevel);
	// {"types": ["Light"], "enemies": ["Boss"], "exclude_types": ["Heavy"], "exclude": ["Fast"]}
	static SpawnRule ReadRule(const model::IConfigNode& InNode);
	// {"delay": 2, "max_alive": 3, "interval": 3, "spawns": [{"spawner": "1", "count": 4}]}
	static Wave ReadWave(const model::IConfigNode& InNode);
	// Unknown class names are logged and skipped.
	static std::vector<EnemyType> ReadEnemyTypes(const model::IConfigNode& InNode, std::string_view InKey);
};

} // namespace tanks
