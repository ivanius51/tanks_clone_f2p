#pragma once

#include "model/MathTypes.h"

namespace tanks::model
{

/** A rectangle of whole map cells, both ends included. */
struct CellRect
{
	int MinX = 0;
	int MinY = 0;
	int MaxX = -1;
	int MaxY = -1;

	bool IsEmpty() const { return MaxX < MinX || MaxY < MinY; }
};

/**
 * What the player can see on a map. Without fog everything is visible; with fog only the square of VisionTiles around the
 * viewer is (Chebyshev distance). One query serves every kind of object, effect and animation: whatever occupies a visible
 * cell is shown. The simulation never asks, only the drawing side does.
 */
class VisibilityMap
{
public:
	VisibilityMap() = default;
	VisibilityMap(int InColumns, int InRows, bool bInFogEnabled, float InVisionTiles);

	// The viewer's position in tiles (a cell's centre is at x + 0.5).
	void SetViewer(const Vec2f& InTilePosition) { Viewer = InTilePosition; }

	bool IsFogEnabled() const { return bFogEnabled; }

	bool IsCellVisible(int InX, int InY) const;

	// True if any cell of InCells is visible; a multi-cell object or an animation shows when part of it does.
	bool IsAreaVisible(const CellRect& InCells) const;

	// The cells to draw: the camera's cells cut to the map and, with fog, to the vision square.
	CellRect GetDrawRect(const CellRect& InCamera) const;

private:
	CellRect GetVisionRect() const;

	int Columns = 0;
	int Rows = 0;
	bool bFogEnabled = false;
	float VisionTiles = 0.0f;
	Vec2f Viewer;
};

} // namespace tanks::model
