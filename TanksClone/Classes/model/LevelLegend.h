#pragma once

#include <string>
#include <vector>

#include "model/ConfigNode.h"
#include "model/GridPath.h"

namespace tanks::model
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
	std::vector<GridCell> Players;
	std::vector<GridCell> Spawners;
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

	const std::vector<LegendEntry>& GetEntries() const { return Entries; }
	const std::vector<std::string>& GetErrors() const { return Errors; }

	const LegendEntry* Find(char InTile) const;
	bool IsKnown(char InTile) const;
	bool IsSpawner(char InTile) const;
	char GetDefaultSpawner() const;

	LegendScan Scan(const std::vector<std::string>& InRows) const;
	GridPath BuildPathGrid(const std::vector<std::string>& InRows, int InWidth) const;

private:
	bool ReadEntries(const IConfigNode& InNode);
	bool ReadRole(const IConfigNode& InNode, const std::string& InTiles, LegendEntry& OutEntry);

	std::vector<LegendEntry> Entries;
	std::vector<std::string> Errors;
};
} // namespace tanks::model
