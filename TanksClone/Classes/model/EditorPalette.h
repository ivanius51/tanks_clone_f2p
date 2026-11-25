#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "model/ConfigNode.h"
#include "model/LevelLegend.h"
#include "templates/TFunction.h"

namespace game::model
{
/** A tab of the editor palette. */
struct EditorTab
{
	std::string Id;
	std::string NameKey;
};

enum class PaletteIcon
{
	Frame,
	TilesetFrame,
	Erase
};

/** One tool of the editor palette: the map character it paints and how it looks. */
struct EditorPaletteEntry
{
	std::size_t Tab = 0;
	char Tile = '.';
	std::string NameKey;
	std::string Frame;
	PaletteIcon Icon = PaletteIcon::Frame;
	float MapScale = 1.0f;
};

/**
 * The editor palette read from configs/editor/palette.json: tabs and the tiles on them.
 * An entry names only a map character; its picture and scale come from the level legend (the object config
 * of that character), so a new wall or bonus appears in the editor without code.
 */
class EditorPalette
{
public:
	using ConfigLookup = TFunction<const IConfigNode*(const std::string&)>;

	bool FromConfig(const IConfigNode& InNode, const LevelLegend& InLegend, const ConfigLookup& InFindConfig);

	const std::vector<EditorTab>& GetTabs() const { return Tabs; }
	const std::vector<EditorPaletteEntry>& GetEntries() const { return Entries; }
	const std::vector<std::string>& GetErrors() const { return Errors; }

	std::vector<std::size_t> GetEntriesOfTab(std::size_t InTab) const;
	const EditorPaletteEntry* FindByTile(char InTile) const;

private:
	bool ReadEntry(const IConfigNode& InNode, const LevelLegend& InLegend, const ConfigLookup& InFindConfig, EditorPaletteEntry& OutEntry);
	bool FindTab(const std::string& InId, std::size_t& OutIndex) const;

	std::vector<EditorTab> Tabs;
	std::vector<EditorPaletteEntry> Entries;
	std::vector<std::string> Errors;
};
} // namespace game::model
