#pragma once

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "templates/TContainers.h"

namespace game::model
{
/** A cell of the level map: row 0 is the top row. */
struct EditorCell
{
	int Row = 0;
	int Column = 0;

	bool operator==(const EditorCell& InOther) const { return Row == InOther.Row && Column == InOther.Column; }
};

/** Which part of the old map stays in place when the map changes size. */
enum class ResizeAnchor
{
	TopLeft,
	Center
};

/**
 * What the editor changes: the map rows, the level properties (name, music, tileset, enemy count, as text) and the
 * script text. Editor commands work on this, never on the level file data directly.
 */
class EditorDocument
{
public:
	static constexpr char EmptyTile = '.';
	static constexpr int MinSide = 3;
	static constexpr int MaxSide = 256;

	EditorDocument() = default;
	EditorDocument(std::vector<std::string> InRows, std::string InUniqueTiles);

	const std::vector<std::string>& GetRows() const { return Rows; }
	int GetRowCount() const { return static_cast<int>(Rows.size()); }
	int GetColumnCount() const { return Rows.empty() ? 0 : static_cast<int>(Rows.front().size()); }
	bool IsInside(const EditorCell& InCell) const;
	char GetTile(const EditorCell& InCell) const;
	bool SetTile(const EditorCell& InCell, char InTile);
	bool IsUnique(char InTile) const;
	void Resize(int InRows, int InColumns, ResizeAnchor InAnchor);
	void SetRows(std::vector<std::string> InRows) { Rows = std::move(InRows); }
	bool FindTile(char InTile, EditorCell& OutCell) const;

	std::string GetProperty(const std::string& InKey) const;
	void SetProperty(const std::string& InKey, const std::string& InValue);

	const std::string& GetScriptText() const { return ScriptText; }
	void SetScriptText(std::string InText) { ScriptText = std::move(InText); }

private:
	std::vector<std::string> Rows;
	std::string UniqueTiles;
	TMap<FPersistentString, std::string> Properties;
	std::string ScriptText;
};
} // namespace game::model
