#include "model/VisibilityMap.h"

#include <algorithm>
#include <cmath>

namespace game::model
{

VisibilityMap::VisibilityMap(const int InColumns, const int InRows, const bool bInFogEnabled, const float InVisionTiles)
	: Columns(std::max(0, InColumns))
	, Rows(std::max(0, InRows))
	, bFogEnabled(bInFogEnabled)
	, VisionTiles(std::max(0.0f, InVisionTiles))
{
}

CellRect VisibilityMap::GetVisionRect() const
{
	if ( !bFogEnabled )
	{
		return { 0, 0, Columns - 1, Rows - 1 };
	}
	// A cell is visible when its centre lies inside the square.
	return {
		static_cast<int>(std::ceil(Viewer.X - VisionTiles - 0.5f)), static_cast<int>(std::ceil(Viewer.Y - VisionTiles - 0.5f)),
		static_cast<int>(std::floor(Viewer.X + VisionTiles - 0.5f)), static_cast<int>(std::floor(Viewer.Y + VisionTiles - 0.5f)) };
}

bool VisibilityMap::IsCellVisible(const int InX, const int InY) const
{
	if ( !bFogEnabled )
	{
		return true;
	}
	const CellRect Vision = GetVisionRect();
	return InX >= Vision.MinX && InX <= Vision.MaxX && InY >= Vision.MinY && InY <= Vision.MaxY;
}

bool VisibilityMap::IsAreaVisible(const CellRect& InCells) const
{
	if ( InCells.IsEmpty() )
	{
		return false;
	}
	if ( !bFogEnabled )
	{
		return true;
	}
	const CellRect Vision = GetVisionRect();
	return InCells.MinX <= Vision.MaxX && InCells.MaxX >= Vision.MinX && InCells.MinY <= Vision.MaxY && InCells.MaxY >= Vision.MinY;
}

CellRect VisibilityMap::GetDrawRect(const CellRect& InCamera) const
{
	const CellRect Vision = GetVisionRect();
	return {
		std::max({ InCamera.MinX, Vision.MinX, 0 }), std::max({ InCamera.MinY, Vision.MinY, 0 }),
		std::min({ InCamera.MaxX, Vision.MaxX, Columns - 1 }), std::min({ InCamera.MaxY, Vision.MaxY, Rows - 1 }) };
}

} // namespace game::model
