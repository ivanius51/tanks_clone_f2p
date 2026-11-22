#pragma once

#include <vector>

namespace tanks::model
{

enum class CellPass
{
	Open,
	Breakable, // a tank gets through after shooting it apart
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

	// The cells to drive through after InFrom, ending with InTo; empty when there is no way or InFrom is InTo.
	std::vector<GridCell> Find(const GridCell& InFrom, const GridCell& InTo, int InBreakCost) const;
	bool IsReachable(const GridCell& InFrom, const GridCell& InTo) const;

private:
	int Index(const GridCell& InCell) const;

	int Width;
	int Height;
	std::vector<CellPass> Cells;
};

} // namespace tanks::model
