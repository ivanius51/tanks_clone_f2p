#pragma once

#include <map>
#include <string>
#include <vector>

#include "core/ObjectEnums.h"
#include "model/AiFeatureSet.h"
#include "model/GridPath.h"
#include "model/JsonValue.h"
#include "model/LevelLegend.h"
#include "model/ScriptPlayer.h"
#include "templates/TContainers.h"

namespace game
{
namespace tiles
{
	constexpr char Empty = '.';
	constexpr char Brick = '#';
	constexpr char Stone = 'S';
	constexpr char Wood = 'W';
	constexpr char PlayerSpawn = 'P';
	constexpr char EnemySpawn = 'E';
	constexpr char Ammo = 'A';
	constexpr char Repair = 'R';
}

/**
 * Says which enemies a spawner may produce. Empty "types" and "enemies" mean "anyone";
 * the exclusions are applied last.
 */
struct SpawnRule
{
	std::vector<EEnemyType> Types;
	std::vector<std::string> Enemies;
	std::vector<EEnemyType> ExcludeTypes;
	std::vector<std::string> ExcludeEnemies;
};

/** A spawner's share of a wave. */
struct WaveEntry
{
	char Spawner = tiles::EnemySpawn;
	int Count = 1;
};

/**
 * One batch of enemies. It starts after the previous wave is destroyed (plus "delay"
 * seconds) and ends when all its enemies have appeared.
 */
struct Wave
{
	float Delay = 2.0f;
	int MaxAlive = 3;
	float Interval = 3.0f;
	std::vector<WaveEntry> Entries;
};

/** Hides the part of the map the player has not seen yet. Read and kept by the model; drawing comes later. */
struct FogOfWar
{
	bool bEnabled = false;
	float VisionTiles = 7.0f;
	bool bShowInitialWalls = true;

	bool operator==(const FogOfWar& InOther) const
	{
		return bEnabled == InOther.bEnabled && VisionTiles == InOther.VisionTiles && bShowInitialWalls == InOther.bShowInitialWalls;
	}
};

/** A level: the map, the simple or the advanced wave form, AI switches, script and fog. */
struct LevelData
{
	std::string Name;
	std::string Music;
	std::string Tileset;
	std::vector<std::string> Rows;

	int GetRowCount() const { return static_cast<int>(Rows.size()); }
	int GetColumnCount() const { return Rows.empty() ? 0 : static_cast<int>(Rows.front().size()); }

	int TotalEnemies = 6;
	int MaxAlive = 3;
	float SpawnInterval = 3.0f;
	std::vector<std::string> EnemyIds;

	TMap<char, SpawnRule> Spawners;
	std::vector<Wave> Waves;

	model::AiSwitches AiFeatures;
	model::Script Script;
	FogOfWar Fog;

	model::JsonValue Extra;
};

/** Loads, validates and writes levels, and builds the path grid of a map. */
class LevelLoader
{
public:
	static int GetBuiltinCount();
	static std::string BuiltinId(int InIndex);

	static bool Load(const std::string& InConfigId, LevelData& InLevel);

	static std::string CustomId();
	static bool HasCustom();
	static bool LoadCustom(LevelData& InLevel);
	static bool SaveCustom(const LevelData& InLevel);

	static LevelData CreateEmpty();
	static std::string Validate(const LevelData& InLevel);
	static std::string Validate(const LevelData& InLevel, const model::LevelLegend& InLegend);
	static model::GridPath BuildPathGrid(const LevelData& InLevel);
	static model::GridPath BuildPathGrid(const LevelData& InLevel, const model::LevelLegend& InLegend);

	static std::vector<std::string> ResolvePool(const SpawnRule& InRule);
	static SpawnRule GetRule(const LevelData& InLevel, char InSpawner);
	static int GetTotalEnemies(const LevelData& InLevel);

	static bool HasOwnPools(const LevelData& InLevel);

	static std::string ToJson(const LevelData& InLevel);
};
} // namespace game
