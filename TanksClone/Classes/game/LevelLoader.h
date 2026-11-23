#pragma once

#include <map>
#include <string>
#include <vector>

#include "core/ObjectEnums.h"
#include "model/AiFeatureSet.h"
#include "model/GridPath.h"
#include "model/LevelLegend.h"
#include "model/ScriptPlayer.h"

namespace tanks
{

// Characters the code itself puts on a map (the simple level form, the editor's starting map, tests).
// The full legend of what each character means lives in configs/levels/legend.json (model::LevelLegend).
namespace tiles
{
	constexpr char Empty = '.';
	constexpr char Brick = '#';
	constexpr char Stone = 'S';
	constexpr char Wood = 'W';
	constexpr char PlayerSpawn = 'P';
	constexpr char EnemySpawn = 'E';  // the default enemy spawner
	constexpr char Ammo = 'A';
	constexpr char Repair = 'R';
}

// Says which enemies a spawner may produce. Empty "types" and "enemies" mean "anyone";
// the exclusions are applied last.
struct SpawnRule
{
	std::vector<EnemyType> Types;            // any enemy of these classes
	std::vector<std::string> Enemies;        // these particular enemies (ids from EnemyCatalog)
	std::vector<EnemyType> ExcludeTypes;     // never enemies of these classes
	std::vector<std::string> ExcludeEnemies; // never these particular enemies
};

// A spawner's share of a wave.
struct WaveEntry
{
	char Spawner = tiles::EnemySpawn; // which spawner sends them
	int Count = 1;
};

// One batch of enemies. It starts after the previous wave is destroyed (plus "delay"
// seconds) and ends when all its enemies have appeared.
struct Wave
{
	float Delay = 2.0f;
	int MaxAlive = 3;       // enemies of the wave on the map at the same time
	float Interval = 3.0f;  // seconds between two enemies
	std::vector<WaveEntry> Entries;
};

struct LevelData
{
	std::string Name;              // shown in the HUD; a string id or plain text
	std::string Music;             // music track id played through the whole level; empty = the calm rotation
	std::string Tileset;           // look of the ground and walls (configs/resources.json); empty = the default
	std::vector<std::string> Rows; // GRID_ROWS rows, the top row first

	// Simple form (what the editor writes): one wave from the default spawner.
	int TotalEnemies = 6;
	int MaxAlive = 3;
	float SpawnInterval = 3.0f;
	std::vector<std::string> EnemyIds; // which enemies may appear (ids from EnemyCatalog)

	// Advanced form: any number of waves and spawners. Used when "waves" is in the level file.
	std::map<char, SpawnRule> Spawners;
	std::vector<Wave> Waves;

	model::AiSwitches AiFeatures; // enemy AI behaviours switched on or off in this level
	model::Script Script;         // a scripted scene at the start of the level; empty = none
};

class LevelLoader
{
public:
	// Levels shipped with the game: configs/levels/campaign/level_1.json, level_2.json, ...
	static int GetBuiltinCount();
	static std::string BuiltinId(int InIndex);

	// Loads a level. The result always has its waves filled in (from the simple form if needed).
	static bool Load(const std::string& InConfigId, LevelData& InLevel);

	// The level made in the editor, stored in the writable folder.
	static std::string CustomId();
	static bool HasCustom();
	static bool LoadCustom(LevelData& InLevel);
	static bool SaveCustom(const LevelData& InLevel);

	// An empty map of the right size.
	static LevelData CreateEmpty();
	// A level needs the right size, known characters, one player spawn and at least one enemy spawner.
	// Returns an empty string if the level is fine, otherwise a string id of the problem.
	static std::string Validate(const LevelData& InLevel);
	static std::string Validate(const LevelData& InLevel, const model::LevelLegend& InLegend);
	// The map as cells for finding a way: stone blocks, walls and other things that can be shot apart are breakable.
	// Cell Y counts rows from the top, like LevelData::Rows.
	static model::GridPath BuildPathGrid(const LevelData& InLevel);
	static model::GridPath BuildPathGrid(const LevelData& InLevel, const model::LevelLegend& InLegend);

	// Enemies a spawner with this rule may produce (ids), after exclusions.
	static std::vector<std::string> ResolvePool(const SpawnRule& InRule);
	// The rule of a spawner: its own, or "the enemies listed in the simple form" if it has none.
	static SpawnRule GetRule(const LevelData& InLevel, char InSpawner);
	static int GetTotalEnemies(const LevelData& InLevel);

	static std::string ToJson(const LevelData& InLevel);
};

} // namespace tanks
