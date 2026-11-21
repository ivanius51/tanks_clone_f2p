#pragma once

#include <cstddef>
#include <vector>

#include "model/MathTypes.h"

namespace tanks::model
{

/** Sizes of the editor toolbar (configs/ui/editor.json, "layout"). The initial values are the fallbacks. */
struct EditorLayoutSettings
{
	float Padding = 8.0f;
	float Gap = 6.0f;
	float MaxTabHeight = 30.0f;
	float MaxItemSize = 48.0f;
	float MinItemSize = 12.0f;
	float MaxActionPanelWidth = 280.0f;
	float ActionPanelShare = 0.4f;
	std::size_t ActionColumns = 2;
};

/** Where the editor toolbar places its panels and buttons. All rects are inside the given bounds. */
struct EditorLayoutResult
{
	RectF PalettePanel;
	RectF ActionPanel;
	std::vector<RectF> Tabs;
	std::vector<RectF> Items;
	std::vector<RectF> Actions;
	RectF CounterMinus;
	RectF CounterLabel;
	RectF CounterPlus;
	bool bItemsFit = true;
};

/**
 * Lays out the editor toolbar: a palette panel (a row of tabs over a grid of tile buttons) on the left
 * and an action panel (action buttons and the enemy counter) on the right. Engine-free, so any
 * screen size can be checked by tests.
 */
class EditorLayout
{
public:
	EditorLayout(std::size_t InActionCount, std::size_t InTabCount, const EditorLayoutSettings& InSettings = EditorLayoutSettings());

	EditorLayoutResult Compute(const RectF& InBounds, std::size_t InItemCount) const;

private:
	void PlaceActions(const RectF& InPanel, EditorLayoutResult& OutResult) const;
	void PlacePalette(const RectF& InPanel, std::size_t InItemCount, EditorLayoutResult& OutResult) const;

	std::size_t ActionCount;
	std::size_t TabCount;
	EditorLayoutSettings Settings;
};

} // namespace tanks::model
