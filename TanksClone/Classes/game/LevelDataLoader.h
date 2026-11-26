#pragma once

#include <string_view>

#include "game/LevelLoader.h"
#include "model/ConfigNode.h"
#include "templates/TContainers.h"

namespace game
{
/** The only place that knows the layout of a level config; LevelLoader gets a plain LevelData. */
class LevelDataLoader
{
public:
	static bool ReadLevel(const model::IConfigNode& InNode, LevelData& OutLevel);
	static std::string WriteLevel(const LevelData& InLevel);
	static Wave MakeSimpleWave(const LevelData& InLevel);
	static bool HasAuthoredWaves(const LevelData& InLevel);
	static SpawnRule ReadRule(const model::IConfigNode& InNode);
	static Wave ReadWave(const model::IConfigNode& InNode);
	static TArray<EEnemyType> ReadEnemyTypes(const model::IConfigNode& InNode, std::string_view InKey);
};
} // namespace game
