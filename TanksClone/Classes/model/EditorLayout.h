#pragma once

#include <cstddef>
#include <vector>
#include "model/MathTypes.h"

namespace tanks::model
{

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
	EditorLayout(std::size_t InActionCount, std::size_t InTabCount);

	EditorLayoutResult Compute(const RectF& InBounds, std::size_t InItemCount) const;

private:
	void PlaceActions(const RectF& InPanel, EditorLayoutResult& OutResult) const;
	void PlacePalette(const RectF& InPanel, std::size_t InItemCount, EditorLayoutResult& OutResult) const;

	std::size_t ActionCount;
	std::size_t TabCount;
};

} // namespace tanks::model
