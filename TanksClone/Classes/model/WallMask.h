#pragma once
#include "model/PhysicsEvents.h"
#include "templates/TContainers.h"


namespace game::model
{
/** The side of a wall a hit comes from. */
enum class HitSide
{
	Bottom,
	Top,
	Left,
	Right
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
	int GetCellHealth(int InColumn, int InRow) const;
	int GetFullCellHealth() const;
	int CountSolid() const;
	bool IsEmpty() const;

	int Chip(HitSide InSide, float InFrom, float InTo, int InDepth, int InDamage, TArray<CellCoord>* OutBroken = nullptr);

	bool Overlaps(float InLeft, float InBottom, float InRight, float InTop) const;

	bool Sweep(float InFromX, float InFromY, float InToX, float InToY, float InHalf, float& OutFraction) const;

private:
	int Index(int InColumn, int InRow) const;
	void CellAt(HitSide InSide, int InLine, int InStep, int& InColumn, int& InRow) const;

	int GridSize;
	int FullHealth;
	TArray<int> Health;
};
} // namespace game::model