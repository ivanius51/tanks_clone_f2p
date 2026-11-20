#pragma once

#include <vector>

namespace tanks::model
{

/** The side of a wall a hit comes from. */
enum class HitSide
{
	Bottom, // the shot flies up
	Top,    // the shot flies down
	Left,   // the shot flies right
	Right   // the shot flies left
};

/**
 * Which parts of a wall tile are still standing: a square grid of sub-cells, each with its own health.
 * A hit chips cells from the side it comes from, so walls crumble layer by layer. No engine types:
 * coordinates are in cells, column 0 on the left, row 0 at the bottom.
 */
class WallMask
{
public:
	WallMask(int InCellsPerSide, int InCellHealth);

	int GetCellsPerSide() const;
	bool IsSolid(int InColumn, int InRow) const;
	// 0 for a removed cell.
	int GetCellHealth(int InColumn, int InRow) const;
	int GetFullCellHealth() const;
	int CountSolid() const;
	bool IsEmpty() const;

	// Every line of cells across the hit (columns for a vertical shot, rows for a horizontal one) that the
	// range [InFrom, InTo) covers takes InDamage in up to InDepth of its first standing cells, counted from the hit side.
	// Returns the number of removed cells.
	int Chip(HitSide InSide, float InFrom, float InTo, int InDepth, int InDamage);

	// True if a standing cell overlaps the box (in cells, edges touching do not count).
	bool Overlaps(float InLeft, float InBottom, float InRight, float InTop) const;

	// Where along the move a -> b a point grown to a box of half size InHalf first touches a standing cell
	// (all in cells). OutFraction is the fraction of the move; returns false if it never does.
	bool Sweep(float InFromX, float InFromY, float InToX, float InToY, float InHalf, float& OutFraction) const;

private:
	int Index(int InColumn, int InRow) const;
	// The cell of line InLine at InStep cells from the hit side.
	void CellAt(HitSide InSide, int InLine, int InStep, int& InColumn, int& InRow) const;

	int GridSize;
	int FullHealth;
	std::vector<int> Health;
};

} // namespace tanks::model