#include "model/EditorDocument.h"

#include <utility>

namespace tanks::model
{

EditorDocument::EditorDocument(std::vector<std::string> InRows, std::string InUniqueTiles)
	: Rows(std::move(InRows))
	, UniqueTiles(std::move(InUniqueTiles))
{
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
	const auto Found = Properties.find(InKey);
	return Found != Properties.end() ? Found->second : std::string();
}

void EditorDocument::SetProperty(const std::string& InKey, const std::string& InValue)
{
	Properties[InKey] = InValue;
}

} // namespace tanks::model
