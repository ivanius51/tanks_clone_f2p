#include "model/EditorDocument.h"
#include "templates/TContainers.h"

#include <algorithm>
#include <utility>

namespace game::model
{

EditorDocument::EditorDocument(TArray<std::string> InRows, std::string InUniqueTiles)
	: Rows(std::move(InRows))
	, UniqueTiles(std::move(InUniqueTiles))
{
}

void EditorDocument::Resize(const int InRows, const int InColumns, const ResizeAnchor InAnchor)
{
	const int NewRows = std::clamp(InRows, MinSide, MaxSide);
	const int NewColumns = std::clamp(InColumns, MinSide, MaxSide);
	const int OldRows = GetRowCount();
	const int OldColumns = GetColumnCount();
	const int ShiftRows = InAnchor == ResizeAnchor::Center ? (NewRows - OldRows) / 2 : 0;
	const int ShiftColumns = InAnchor == ResizeAnchor::Center ? (NewColumns - OldColumns) / 2 : 0;
	TArray<std::string> Result(static_cast<std::size_t>(NewRows), std::string(static_cast<std::size_t>(NewColumns), EmptyTile));
	for ( int Row = 0; Row < NewRows; ++Row )
	{
		for ( int Column = 0; Column < NewColumns; ++Column )
		{
			const EditorCell Old = { Row - ShiftRows, Column - ShiftColumns };
			if ( IsInside(Old) )
			{
				Result[Row][Column] = GetTile(Old);
			}
		}
	}
	Rows = std::move(Result);
}

bool EditorDocument::IsInside(const EditorCell& InCell) const
{
	return InCell.Row >= 0 && InCell.Row < GetRowCount() && InCell.Column >= 0 && InCell.Column < static_cast<int>(Rows[InCell.Row].size());
}

char EditorDocument::GetTile(const EditorCell& InCell) const
{
	return IsInside(InCell) ? Rows[InCell.Row][InCell.Column] : EmptyTile;
}

bool EditorDocument::SetTile(const EditorCell& InCell, const char InTile)
{
	if ( !IsInside(InCell) )
	{
		return false;
	}
	Rows[InCell.Row][InCell.Column] = InTile;
	return true;
}

bool EditorDocument::IsUnique(const char InTile) const
{
	return UniqueTiles.find(InTile) != std::string::npos;
}

bool EditorDocument::FindTile(const char InTile, EditorCell& OutCell) const
{
	for ( int Row = 0; Row < GetRowCount(); ++Row )
	{
		const std::size_t Column = Rows[Row].find(InTile);
		if ( Column != std::string::npos )
		{
			OutCell = EditorCell{ Row, static_cast<int>(Column) };
			return true;
		}
	}
	return false;
}

std::string EditorDocument::GetProperty(const std::string& InKey) const
{
	auto* const Found = Properties.Find(InKey);
	return Found ? *Found : std::string();
}

void EditorDocument::SetProperty(const std::string& InKey, const std::string& InValue)
{
	Properties[InKey] = InValue;
}

} // namespace game::model
