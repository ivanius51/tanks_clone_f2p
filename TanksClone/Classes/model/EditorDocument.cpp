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

	if ( !Floor.IsEmpty() )
	{
		TArray<std::string> ResizedFloor;
		for ( int Row = 0; Row < NewRows; ++Row )
		{
			std::string Line(static_cast<std::size_t>(NewColumns), EmptyTile);
			const int OldRow = Row - ShiftRows;
			for ( int Column = 0; Column < NewColumns; ++Column )
			{
				const int OldColumn = Column - ShiftColumns;
				const bool bInside = OldRow >= 0 && OldRow < static_cast<int>(Floor.Size()) && OldColumn >= 0 && OldColumn < static_cast<int>(Floor[OldRow].size());
				Line[Column] = bInside ? Floor[OldRow][OldColumn] : EmptyTile;
			}
			ResizedFloor.PushBack(std::move(Line));
		}
		Floor = std::move(ResizedFloor);
	}
	Layers.Crop(NewColumns, NewRows, ShiftColumns, ShiftRows);
}

char EditorDocument::GetFloorTile(const EditorCell& InCell) const
{
	const bool bInside = InCell.Row >= 0 && InCell.Row < static_cast<int>(Floor.Size()) && InCell.Column >= 0 && InCell.Column < static_cast<int>(Floor[InCell.Row].size());
	return bInside ? Floor[InCell.Row][InCell.Column] : EmptyTile;
}

bool EditorDocument::SetFloorTile(const EditorCell& InCell, const char InFloor)
{
	if ( !IsInside(InCell) )
	{
		return false;
	}
	if ( Floor.IsEmpty() )
	{
		Floor = TArray<std::string>(static_cast<std::size_t>(GetRowCount()), std::string(static_cast<std::size_t>(GetColumnCount()), EmptyTile));
	}
	Floor[InCell.Row][InCell.Column] = InFloor;
	const bool bAnyFloor = std::any_of(Floor.begin(), Floor.end(), [](const std::string& InLine) { return InLine.find_first_not_of(EmptyTile) != std::string::npos; });
	if ( !bAnyFloor )
	{
		Floor.Clear();
	}
	return true;
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
