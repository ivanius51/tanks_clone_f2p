#pragma once

#include "model/MathTypes.h"

namespace game::model
{
/** Sizes of the editor docks and map (configs/ui/editor.json, "layout"). The initial values are the fallbacks. */
struct EditorLayoutSettings
{
	float Padding = 8.0f;
	float Gap = 6.0f;
	float SideWidthShare = 0.22f;
	float MinSideWidth = 220.0f;
	float MaxSideWidth = 340.0f;
	float BottomHeightShare = 0.26f;
	float MinBottomHeight = 120.0f;
	float MaxBottomHeight = 240.0f;
	float CollapsedSize = 32.0f;
	float ScrollbarSize = 14.0f;
	float MinimapSize = 150.0f;
	float CollapseRightBelowWidth = 1100.0f;
	float CollapseLeftBelowWidth = 800.0f;
};

/** Which docks the user collapsed. */
struct EditorDockState
{
	bool bLeftCollapsed = false;
	bool bRightCollapsed = false;
	bool bBottomCollapsed = false;
	bool bAutoCollapse = true;
};

/** Where the editor places its docks, map view, scrollbars and minimap. All rects are inside the given bounds. */
struct EditorLayoutResult
{
	RectF LeftDock;
	RectF RightDock;
	RectF BottomDock;
	RectF MapView;
	RectF VerticalBar;
	RectF HorizontalBar;
	RectF Minimap;
	bool bLeftCollapsed = false;
	bool bRightCollapsed = false;
	bool bBottomCollapsed = false;
};

/**
 * Lays out the editor screen: a left and a right dock over the full height, a bottom dock between them, and the map
 * (with its scrollbars and minimap) in the rest. A collapsed dock shrinks to a thin strip that keeps its handle.
 * Engine-free, so any screen size can be checked by tests.
 */
class EditorLayout
{
public:
	explicit EditorLayout(const EditorLayoutSettings& InSettings = EditorLayoutSettings());

	EditorLayoutResult Compute(const RectF& InBounds, const EditorDockState& InState) const;

private:
	float SideWidth(float InBoundsWidth) const;
	float BottomHeight(float InBoundsHeight) const;

	EditorLayoutSettings Settings;
};
} // namespace game::model
