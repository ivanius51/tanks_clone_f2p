#include "model/EditorLayout.h"

#include <algorithm>

namespace tanks::model
{

EditorLayout::EditorLayout(const EditorLayoutSettings& InSettings)
	: Settings(InSettings)
{
}

float EditorLayout::SideWidth(const float InBoundsWidth) const
{
	return std::clamp(InBoundsWidth * Settings.SideWidthShare, Settings.MinSideWidth, Settings.MaxSideWidth);
}

float EditorLayout::BottomHeight(const float InBoundsHeight) const
{
	return std::clamp(InBoundsHeight * Settings.BottomHeightShare, Settings.MinBottomHeight, Settings.MaxBottomHeight);
}

EditorLayoutResult EditorLayout::Compute(const RectF& InBounds, const EditorDockState& InState) const
{
	EditorLayoutResult Result;
	Result.bLeftCollapsed = InState.bLeftCollapsed || (InState.bAutoCollapse && InBounds.Size.X < Settings.CollapseLeftBelowWidth);
	Result.bRightCollapsed = InState.bRightCollapsed || (InState.bAutoCollapse && InBounds.Size.X < Settings.CollapseRightBelowWidth);
	Result.bBottomCollapsed = InState.bBottomCollapsed;

	const float LeftWidth = Result.bLeftCollapsed ? Settings.CollapsedSize : SideWidth(InBounds.Size.X);
	const float RightWidth = Result.bRightCollapsed ? Settings.CollapsedSize : SideWidth(InBounds.Size.X);
	const float BottomSize = Result.bBottomCollapsed ? Settings.CollapsedSize : BottomHeight(InBounds.Size.Y);

	Result.LeftDock = RectF(InBounds.GetMinX(), InBounds.GetMinY(), LeftWidth, InBounds.Size.Y);
	Result.RightDock = RectF(InBounds.GetMaxX() - RightWidth, InBounds.GetMinY(), RightWidth, InBounds.Size.Y);
	const float CentreLeft = Result.LeftDock.GetMaxX();
	const float CentreWidth = std::max(0.0f, Result.RightDock.GetMinX() - CentreLeft);
	Result.BottomDock = RectF(CentreLeft, InBounds.GetMinY(), CentreWidth, BottomSize);

	const float AreaBottom = Result.BottomDock.GetMaxY();
	const float AreaHeight = std::max(0.0f, InBounds.GetMaxY() - AreaBottom);
	const float Bar = Settings.ScrollbarSize;
	Result.MapView = RectF(CentreLeft, AreaBottom + Bar, std::max(0.0f, CentreWidth - Bar), std::max(0.0f, AreaHeight - Bar));
	Result.HorizontalBar = RectF(CentreLeft, AreaBottom, Result.MapView.Size.X, Bar);
	Result.VerticalBar = RectF(Result.MapView.GetMaxX(), AreaBottom + Bar, Bar, Result.MapView.Size.Y);

	const float MinimapSide = std::min(Settings.MinimapSize, std::min(Result.MapView.Size.X, Result.MapView.Size.Y) / 2.0f);
	Result.Minimap = RectF(Result.MapView.GetMaxX() - MinimapSide - Settings.Padding, Result.MapView.GetMinY() + Settings.Padding, MinimapSide, MinimapSide);
	return Result;
}

} // namespace tanks::model
