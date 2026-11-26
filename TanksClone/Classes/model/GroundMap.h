#pragma once

#include <string>

#include "model/LevelLegend.h"
#include "templates/TContainers.h"

namespace game::model
{
/** What a floor cell does to a tank driving over it. */
struct GroundEffect
{
	float SpeedScale = 1.0f;
	bool bLeavesMarks = true;
};

/**
 * The floor effects of a level (roads, mud) built from its `ground` rows and the legend. Cells outside the map and
 * plain ground have no effect. Rows are counted from the top of the map, like `rows`. Engine-free.
 */
class GroundMap
{
public:
	void Build(const TArray<std::string>& InGround, const LevelLegend& InLegend, int InColumns, int InRows);
	void Clear();

	bool IsEmpty() const { return Cells.IsEmpty(); }
	GroundEffect GetEffect(int InColumn, int InRow) const;
	// World position in tiles measured from the bottom-left corner (the scene's Y goes up).
	GroundEffect GetEffectAtWorld(float InX, float InY, float InTileSize) const;

private:
	int Columns = 0;
	int Rows = 0;
	TArray<GroundEffect> Cells;
};
} // namespace game::model
