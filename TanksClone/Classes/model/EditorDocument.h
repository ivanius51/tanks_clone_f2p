#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace tanks::model
{

/** A cell of the level map: row 0 is the top row. */
struct EditorCell
{
	int Row = 0;
	int Column = 0;

	bool operator==(const EditorCell& InOther) const { return Row == InOther.Row && Column == InOther.Column; }
};

/**
 * What the editor changes: the map rows, the level properties (name, music, tileset, enemy count, as text) and the
 * script text. Editor commands work on this, never on the level file data directly.
 */
class EditorDocument
{
public:
	static constexpr char EmptyTile = '.';

	EditorDocument() = default;
	EditorDocument(std::vector<std::string> InRows, std::string InUniqueTiles);

	const std::vector<std::string>& GetRows() const { return Rows; }
	int GetRowCount() const { return static_cast<int>(Rows.size()); }
	int GetColumnCount() const { return Rows.empty() ? 0 : static_cast<int>(Rows.front().size()); }
	bool IsInside(const EditorCell& InCell) const;
	// The character at the cell, or EmptyTile outside the map.
	char GetTile(const EditorCell& InCell) const;
	// False outside the map.
	bool SetTile(const EditorCell& InCell, char InTile);
	bool IsUnique(char InTile) const;
	// The first cell holding the tile, if any.
	bool FindTile(char InTile, EditorCell& OutCell) const;

	std::string GetProperty(const std::string& InKey) const;
	void SetProperty(const std::string& InKey, const std::string& InValue);

	const std::string& GetScriptText() const { return ScriptText; }
	void SetScriptText(std::string InText) { ScriptText = std::move(InText); }

private:
	std::vector<std::string> Rows;
	std::string UniqueTiles;
	std::map<std::string, std::string> Properties;
	std::string ScriptText;
};

} // namespace tanks::model
