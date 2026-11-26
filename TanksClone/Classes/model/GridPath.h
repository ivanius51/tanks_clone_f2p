#pragma once
#include "templates/TContainers.h"


namespace game::model
{
enum class CellPass
{
	Open,
	Breakable,
	Blocked
};

struct GridCell
{
	int X = 0;
	int Y = 0;

	bool operator==(const GridCell& InOther) const { return X == InOther.X && Y == InOther.Y; }
	bool operator!=(const GridCell& InOther) const { return !(*this == InOther); }
};

/**
 * A map grid where tanks drive between neighbouring cells. Finds the cheapest way between two cells: an open cell
 * costs 1, a breakable one costs more (it has to be shot first), a blocked one is never entered.
 */
class GridPath
{
public:
	GridPath(int InWidth, int InHeight);

	int GetWidth() const;
	int GetHeight() const;
	bool Contains(const GridCell& InCell) const;
	void Set(const GridCell& InCell, CellPass InPass);
	CellPass Get(const GridCell& InCell) const;

	TArray<GridCell> Find(const GridCell& InFrom, const GridCell& InTo, int InBreakCost) const;
	bool IsReachable(const GridCell& InFrom, const GridCell& InTo) const;

private:
	int Index(const GridCell& InCell) const;

	int Width;
	int Height;
	TArray<CellPass> Cells;
};
} // namespace game::model
