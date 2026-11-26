#pragma once

#include <string>
#include <string_view>

#include "model/ConfigNode.h"
#include "model/JsonValue.h"
#include "templates/TContainers.h"

namespace game::model
{
/** A named cell of the map that spawners, script actors and test setups link to by name. */
struct LevelPoint
{
	std::string Name;
	int Column = 0;
	int Row = 0;
};

/** A single object (a prop, a start of player 2) on a cell, made from a config. */
struct LevelObject
{
	std::string Config;
	std::string Name;
	int Column = 0;
	int Row = 0;
};

/**
 * The optional layers of a level above the ground rows: sparse objects and named points.
 * Both are written only when present, so older levels load and save unchanged.
 */
class LevelLayers
{
public:
	static LevelLayers FromConfig(const IConfigNode& InNode);
	JsonValue ObjectsToJson() const;
	JsonValue PointsToJson() const;

	bool IsEmpty() const { return Points.IsEmpty() && Objects.IsEmpty(); }
	const TArray<LevelPoint>& GetPoints() const { return Points; }
	const TArray<LevelObject>& GetObjects() const { return Objects; }

	const LevelPoint* FindPoint(std::string_view InName) const;
	// Adds a point, or moves the one with this name; an empty name is refused.
	bool SetPoint(std::string_view InName, int InColumn, int InRow);
	bool RemovePoint(std::string_view InName);
	std::string MakePointName() const;

	void AddObject(const LevelObject& InObject);
	// Removes the topmost object on the cell; false when there is none.
	bool RemoveObjectAt(int InColumn, int InRow);

	// Names from InLinks that no point has; a script or spawner linking to them is broken.
	TArray<std::string> FindMissingPoints(const TArray<std::string>& InLinks) const;

	// Keeps only what lies inside the new size (columns, rows) and moves it by the shift of a resize.
	void Crop(int InColumns, int InRows, int InShiftColumns, int InShiftRows);

private:
	TArray<LevelPoint> Points;
	TArray<LevelObject> Objects;
};
} // namespace game::model
