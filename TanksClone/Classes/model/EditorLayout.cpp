#include "model/EditorLayout.h"

#include <algorithm>

namespace tanks::model
{

namespace
{
	struct GridFit
	{
		std::size_t Columns = 1;
		float Size = 0.0f;
		bool bFits = false;
	};

	std::size_t CellsAlong(const float InLength, const float InSize, const float InGap)
	{
		return InLength < InSize ? 0 : static_cast<std::size_t>((InLength + InGap) / (InSize + InGap));
	}

	GridFit FitGrid(const float InWidth, const float InHeight, const std::size_t InCount, const EditorLayoutSettings& InSettings)
	{
		const float Largest = std::min(InSettings.MaxItemSize, std::min(InWidth, InHeight));
		for ( float Size = Largest; Size >= InSettings.MinItemSize; Size -= 1.0f )
		{
			const std::size_t Columns = CellsAlong(InWidth, Size, InSettings.Gap);
			const std::size_t Rows = CellsAlong(InHeight, Size, InSettings.Gap);
			if ( Columns > 0 && Columns * Rows >= InCount )
			{
				return { Columns, Size, true };
			}
		}
		GridFit Result;
		Result.Size = InSettings.MinItemSize;
		Result.Columns = std::max<std::size_t>(1, CellsAlong(InWidth, InSettings.MinItemSize, InSettings.Gap));
		return Result;
	}
}

EditorLayout::EditorLayout(const std::size_t InActionCount, const std::size_t InTabCount, const EditorLayoutSettings& InSettings)
	: ActionCount(InActionCount), TabCount(std::max<std::size_t>(1, InTabCount)), Settings(InSettings)
{
}

EditorLayoutResult EditorLayout::Compute(const RectF& InBounds, const std::size_t InItemCount) const
{
	EditorLayoutResult Result;
	const RectF Inner(InBounds.GetMinX() + Settings.Padding, InBounds.GetMinY() + Settings.Padding,
		std::max(0.0f, InBounds.Size.X - 2 * Settings.Padding), std::max(0.0f, InBounds.Size.Y - 2 * Settings.Padding));

	const float ActionWidth = std::min(Settings.MaxActionPanelWidth, Inner.Size.X * Settings.ActionPanelShare);
	Result.ActionPanel = RectF(Inner.GetMaxX() - ActionWidth, Inner.GetMinY(), ActionWidth, Inner.Size.Y);
	Result.PalettePanel = RectF(Inner.GetMinX(), Inner.GetMinY(), std::max(0.0f, Inner.Size.X - ActionWidth - Settings.Gap), Inner.Size.Y);

	PlaceActions(Result.ActionPanel, Result);
	PlacePalette(Result.PalettePanel, InItemCount, Result);
	return Result;
}

void EditorLayout::PlaceActions(const RectF& InPanel, EditorLayoutResult& OutResult) const
{
	const std::size_t ActionRows = (ActionCount + Settings.ActionColumns - 1) / Settings.ActionColumns;
	const std::size_t Rows = ActionRows + 1;
	const float RowHeight = std::max(0.0f, (InPanel.Size.Y - Settings.Gap * (Rows + 1)) / Rows);
	const float ColumnWidth = std::max(0.0f, (InPanel.Size.X - Settings.Gap * (Settings.ActionColumns + 1)) / Settings.ActionColumns);

	for ( std::size_t Index = 0; Index < ActionCount; ++Index )
	{
		const std::size_t Row = Index / Settings.ActionColumns;
		const std::size_t Column = Index % Settings.ActionColumns;
		const float X = InPanel.GetMinX() + Settings.Gap + Column * (ColumnWidth + Settings.Gap);
		const float Y = InPanel.GetMaxY() - (Row + 1) * (RowHeight + Settings.Gap);
		OutResult.Actions.emplace_back(X, Y, ColumnWidth, RowHeight);
	}

	const float CounterY = InPanel.GetMinY() + Settings.Gap;
	const float Square = std::min(RowHeight, InPanel.Size.X / 4);
	OutResult.CounterMinus = RectF(InPanel.GetMinX() + Settings.Gap, CounterY, Square, RowHeight);
	OutResult.CounterPlus = RectF(InPanel.GetMaxX() - Settings.Gap - Square, CounterY, Square, RowHeight);
	const float LabelX = OutResult.CounterMinus.GetMaxX() + Settings.Gap;
	OutResult.CounterLabel = RectF(LabelX, CounterY, std::max(0.0f, OutResult.CounterPlus.GetMinX() - Settings.Gap - LabelX), RowHeight);
}

void EditorLayout::PlacePalette(const RectF& InPanel, const std::size_t InItemCount, EditorLayoutResult& OutResult) const
{
	const float TabHeight = std::min(Settings.MaxTabHeight, InPanel.Size.Y / 3);
	const float TabWidth = std::max(0.0f, (InPanel.Size.X - Settings.Gap * (TabCount + 1)) / TabCount);
	const float TabY = InPanel.GetMaxY() - Settings.Gap - TabHeight;
	for ( std::size_t Index = 0; Index < TabCount; ++Index )
	{
		OutResult.Tabs.emplace_back(InPanel.GetMinX() + Settings.Gap + Index * (TabWidth + Settings.Gap), TabY, TabWidth, TabHeight);
	}

	const RectF Area(InPanel.GetMinX() + Settings.Gap, InPanel.GetMinY() + Settings.Gap,
		std::max(0.0f, InPanel.Size.X - 2 * Settings.Gap), std::max(0.0f, TabY - Settings.Gap - InPanel.GetMinY() - Settings.Gap));
	const GridFit Fit = FitGrid(Area.Size.X, Area.Size.Y, InItemCount, Settings);
	OutResult.bItemsFit = Fit.bFits;
	for ( std::size_t Index = 0; Index < InItemCount; ++Index )
	{
		const std::size_t Row = Index / Fit.Columns;
		const std::size_t Column = Index % Fit.Columns;
		const float X = Area.GetMinX() + Column * (Fit.Size + Settings.Gap);
		const float Y = Area.GetMaxY() - (Row + 1) * Fit.Size - Row * Settings.Gap;
		OutResult.Items.emplace_back(X, Y, Fit.Size, Fit.Size);
	}
}

} // namespace tanks::model
