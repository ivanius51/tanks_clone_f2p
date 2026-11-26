#pragma once

#include <string>

#include "model/ConfigNode.h"
#include "model/GridPath.h"
#include "templates/TContainers.h"

namespace game::model
{
enum class TileRole
{
	Empty,
	Wall,
	Prop,
	Pickup,
	Crate,
	Player,
	Spawner
};

/** One character of the level map: what it is, which object config it spawns and what the code needs to know about it. */
struct LegendEntry
{
	char Tile = '.';
	TileRole Role = TileRole::Empty;
	std::string Config;
	std::string Frame;
	float MapScale = 1.0f;
	CellPass Pass = CellPass::Open;
	bool bUnique = false;
	bool bIsBase = false;
	bool bDefaultSpawner = false;
};

/** What a map holds, counted through the legend. */
struct LegendScan
{
	bool bUnknownTile = false;
	bool bUniqueRepeated = false;
	TArray<GridCell> Players;
	TArray<GridCell> Spawners;
	std::string SpawnerTiles;
};

/**
 * The legend of the level map read from configs/levels/legend.json: every character allowed in `rows`, its role and
 * flags. Level validation, the path grid, scene spawning and the editor palette all read this one table, so a new
 * object is added in data only.
 */
class LevelLegend
{
public:
	bool FromConfig(const IConfigNode& InNode);

	const TArray<LegendEntry>& GetEntries() const { return Entries; }
	const TArray<std::string>& GetErrors() const { return Errors; }

	const LegendEntry* Find(char InTile) const;
	bool IsKnown(char InTile) const;
	bool IsSpawner(char InTile) const;
	char GetDefaultSpawner() const;

	LegendScan Scan(const TArray<std::string>& InRows) const;
	GridPath BuildPathGrid(const TArray<std::string>& InRows, int InWidth) const;

private:
	bool ReadEntries(const IConfigNode& InNode);
	bool ReadRole(const IConfigNode& InNode, const std::string& InTiles, LegendEntry& OutEntry);

	TArray<LegendEntry> Entries;
	TArray<std::string> Errors;
};
} // namespace game::model
