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
	Wall,    // a solid object spawned from its config
	Prop,    // any other object spawned from its config
	Pickup,  // a pickup lying in a supply crate
	Crate,   // a supply crate with a random pickup
	Player,  // where the player starts
	Spawner  // where enemies appear
};

/** One character of the level map: what it is, which object config it spawns and what the code needs to know about it. */
struct LegendEntry
{
	char Tile = '.';
	TileRole Role = TileRole::Empty;
	std::string Config;  // object config id (resource id); empty for roles that spawn nothing
	std::string Frame;   // editor picture for entries without a config
	float MapScale = 1.0f;
	CellPass Pass = CellPass::Open;
	bool bUnique = false;      // at most one on a map
	bool bIsBase = false;      // destroying it loses the level
	bool bDefaultSpawner = false;
};

/** What a map holds, counted through the legend. */
struct LegendScan
{
	bool bUnknownTile = false;
	bool bUniqueRepeated = false; // a unique tile other than the player start appears twice (players are counted instead)
	std::vector<GridCell> Players;
	std::vector<GridCell> Spawners;
	std::string SpawnerTiles;
};

/**
 * The legend of the level map read from configs/level_legend.json: every character allowed in `rows`, its role and
 * flags. Level validation, the path grid, scene spawning and the editor palette all read this one table, so a new
 * object is added in data only.
 */
class LevelLegend
{
public:
	// False when no valid entry was read; GetErrors() says what was skipped.
	bool FromConfig(const IConfigNode& InNode);

	const std::vector<LegendEntry>& GetEntries() const { return Entries; }
	const std::vector<std::string>& GetErrors() const { return Errors; }

	// nullptr for a character that is not in the legend.
	const LegendEntry* Find(char InTile) const;
	bool IsKnown(char InTile) const;
	bool IsSpawner(char InTile) const;
	char GetDefaultSpawner() const;

	LegendScan Scan(const std::vector<std::string>& InRows) const;
	// Cell Y counts rows from the top. Unknown characters count as open.
	GridPath BuildPathGrid(const std::vector<std::string>& InRows, int InWidth) const;

private:
	bool ReadEntries(const IConfigNode& InNode);
	bool ReadRole(const IConfigNode& InNode, const std::string& InTiles, LegendEntry& OutEntry);

	std::vector<LegendEntry> Entries;
	std::vector<std::string> Errors;
};

} // namespace tanks::model
