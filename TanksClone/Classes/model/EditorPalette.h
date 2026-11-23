#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "model/ConfigNode.h"
#include "model/LevelLegend.h"

namespace tanks::model
{

/** A tab of the editor palette. */
struct EditorTab
{
	std::string Id;
	std::string NameKey;
};

enum class PaletteIcon
{
	Frame,        // a sprite frame shared by all tilesets
	TilesetFrame, // a frame that every tileset draws its own way
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
	using ConfigLookup = std::function<const IConfigNode*(const std::string&)>;

	// InFindConfig maps an object config id (as written in the legend) to its node.
	// False when there are no tabs or no valid entries; GetErrors() says what was skipped.
	bool FromConfig(const IConfigNode& InNode, const LevelLegend& InLegend, const ConfigLookup& InFindConfig);

	const std::vector<EditorTab>& GetTabs() const { return Tabs; }
	const std::vector<EditorPaletteEntry>& GetEntries() const { return Entries; }
	const std::vector<std::string>& GetErrors() const { return Errors; }

	// Indices into GetEntries(), in config order.
	std::vector<std::size_t> GetEntriesOfTab(std::size_t InTab) const;
	// nullptr when no entry paints this character.
	const EditorPaletteEntry* FindByTile(char InTile) const;

private:
	bool ReadEntry(const IConfigNode& InNode, const LevelLegend& InLegend, const ConfigLookup& InFindConfig, EditorPaletteEntry& OutEntry);
	bool FindTab(const std::string& InId, std::size_t& OutIndex) const;

	std::vector<EditorTab> Tabs;
	std::vector<EditorPaletteEntry> Entries;
	std::vector<std::string> Errors;
};

} // namespace tanks::model
