#pragma once

#include <string>

#include "model/ConfigNode.h"
#include "model/GridPath.h"
#include "model/MathTypes.h"
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

/** One floor tile of the optional `ground` layer: a road or mud that changes how fast tanks drive and whether they leave marks. */
struct GroundEntry
{
	char Tile = '.';
	std::string NameKey;
	// Tileset frame name without the variant suffix: "floor_road" is drawn from floor_road_1 or floor_road_2.
	std::string Frame;
	Color Tint{ 255, 255, 255 };
	float SpeedScale = 1.0f;
	bool bLeavesMarks = true;
	bool bBlocksMovement = false;
	FPersistentString PassBuff;
	// A tank standing here is unseen by the AI farther than this many tiles until it shoots; 0 = no cover.
	float ConcealSightTiles = 0.0f;
	float ConcealRevealTime = 0.0f;
	float ConcealOpacity = 1.0f;
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
	const TArray<GroundEntry>& GetGroundEntries() const { return Grounds; }
	const TArray<std::string>& GetErrors() const { return Errors; }

	const GroundEntry* FindGround(char InTile) const;
	const LegendEntry* Find(char InTile) const;
	bool IsKnown(char InTile) const;
	bool IsSpawner(char InTile) const;
	char GetDefaultSpawner() const;

	LegendScan Scan(const TArray<std::string>& InRows) const;
	GridPath BuildPathGrid(const TArray<std::string>& InRows, int InWidth) const;

private:
	bool ReadEntries(const IConfigNode& InNode);
	bool ReadRole(const IConfigNode& InNode, const std::string& InTiles, LegendEntry& OutEntry);

	void ReadGrounds(const IConfigNode& InNode);

	TArray<LegendEntry> Entries;
	TArray<GroundEntry> Grounds;
	TArray<std::string> Errors;
};
} // namespace game::model
