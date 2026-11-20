#include "model/EditorLayout.h"

#include <algorithm>

namespace tanks::model
{

namespace
{
	constexpr float Padding = 8.0f;
	constexpr float Gap = 6.0f;
	constexpr float MaxTabHeight = 30.0f;
	constexpr float MaxItemSize = 48.0f;
	constexpr float MinItemSize = 12.0f;
	constexpr float MaxActionPanelWidth = 280.0f;
	constexpr float ActionPanelShare = 0.4f;
	constexpr std::size_t ActionColumns = 2;

	struct GridFit
	{
		std::size_t Columns = 1;
		float Size = MinItemSize;
		bool bFits = false;
	};

	std::size_t CellsAlong(const float InLength, const float InSize)
	{
		return InLength < InSize ? 0 : static_cast<std::size_t>((InLength + Gap) / (InSize + Gap));
	}

	GridFit FitGrid(const float InWidth, const float InHeight, const std::size_t InCount)
	{
		const float Largest = std::min(MaxItemSize, std::min(InWidth, InHeight));
		for ( float Size = Largest; Size >= MinItemSize; Size -= 1.0f )
		{
			const std::size_t Columns = CellsAlong(InWidth, Size);
			const std::size_t Rows = CellsAlong(InHeight, Size);
			if ( Columns > 0 && Columns * Rows >= InCount )
			{
				return { Columns, Size, true };
			}
		}
		GridFit Result;
		Result.Columns = std::max<std::size_t>(1, CellsAlong(InWidth, MinItemSize));
		return Result;
	}
}

EditorLayout::EditorLayout(const std::size_t InActionCount, const std::size_t InTabCount)
	: ActionCount(InActionCount), TabCount(std::max<std::size_t>(1, InTabCount))
{
}

EditorLayoutResult EditorLayout::Compute(const RectF& InBounds, const std::size_t InItemCount) const
{
	EditorLayoutResult Result;
	const RectF Inner(InBounds.GetMinX() + Padding, InBounds.GetMinY() + Padding,
		std::max(0.0f, InBounds.Size.X - 2 * Padding), std::max(0.0f, InBounds.Size.Y - 2 * Padding));

	const float ActionWidth = std::min(MaxActionPanelWidth, Inner.Size.X * ActionPanelShare);
	Result.ActionPanel = RectF(Inner.GetMaxX() - ActionWidth, Inner.GetMinY(), ActionWidth, Inner.Size.Y);
	Result.PalettePanel = RectF(Inner.GetMinX(), Inner.GetMinY(), std::max(0.0f, Inner.Size.X - ActionWidth - Gap), Inner.Size.Y);

	PlaceActions(Result.ActionPanel, Result);
	PlacePalette(Result.PalettePanel, InItemCount, Result);
	return Result;
}

void EditorLayout::PlaceActions(const RectF& InPanel, EditorLayoutResult& OutResult) const
{
	const std::size_t ActionRows = (ActionCount + ActionColumns - 1) / ActionColumns;
	const std::size_t Rows = ActionRows + 1;
	const float RowHeight = std::max(0.0f, (InPanel.Size.Y - Gap * (Rows + 1)) / Rows);
	const float ColumnWidth = std::max(0.0f, (InPanel.Size.X - Gap * (ActionColumns + 1)) / ActionColumns);

	for ( std::size_t Index = 0; Index < ActionCount; ++Index )
	{
		const std::size_t Row = Index / ActionColumns;
		const std::size_t Column = Index % ActionColumns;
		const float X = InPanel.GetMinX() + Gap + Column * (ColumnWidth + Gap);
		const float Y = InPanel.GetMaxY() - (Row + 1) * (RowHeight + Gap);
		OutResult.Actions.emplace_back(X, Y, ColumnWidth, RowHeight);
	}

	const float CounterY = InPanel.GetMinY() + Gap;
	const float Square = std::min(RowHeight, InPanel.Size.X / 4);
	OutResult.CounterMinus = RectF(InPanel.GetMinX() + Gap, CounterY, Square, RowHeight);
	OutResult.CounterPlus = RectF(InPanel.GetMaxX() - Gap - Square, CounterY, Square, RowHeight);
	const float LabelX = OutResult.CounterMinus.GetMaxX() + Gap;
	OutResult.CounterLabel = RectF(LabelX, CounterY, std::max(0.0f, OutResult.CounterPlus.GetMinX() - Gap - LabelX), RowHeight);
}

void EditorLayout::PlacePalette(const RectF& InPanel, const std::size_t InItemCount, EditorLayoutResult& OutResult) const
{
	const float TabHeight = std::min(MaxTabHeight, InPanel.Size.Y / 3);
	const float TabWidth = std::max(0.0f, (InPanel.Size.X - Gap * (TabCount + 1)) / TabCount);
	const float TabY = InPanel.GetMaxY() - Gap - TabHeight;
	for ( std::size_t Index = 0; Index < TabCount; ++Index )
	{
		OutResult.Tabs.emplace_back(InPanel.GetMinX() + Gap + Index * (TabWidth + Gap), TabY, TabWidth, TabHeight);
	}

	const RectF Area(InPanel.GetMinX() + Gap, InPanel.GetMinY() + Gap,
		std::max(0.0f, InPanel.Size.X - 2 * Gap), std::max(0.0f, TabY - Gap - InPanel.GetMinY() - Gap));
	const GridFit Fit = FitGrid(Area.Size.X, Area.Size.Y, InItemCount);
	OutResult.bItemsFit = Fit.bFits;
	for ( std::size_t Index = 0; Index < InItemCount; ++Index )
	{
		const std::size_t Row = Index / Fit.Columns;
		const std::size_t Column = Index % Fit.Columns;
		const float X = Area.GetMinX() + Column * (Fit.Size + Gap);
		const float Y = Area.GetMaxY() - (Row + 1) * Fit.Size - Row * Gap;
		OutResult.Items.emplace_back(X, Y, Fit.Size, Fit.Size);
	}
}

} // namespace tanks::model
