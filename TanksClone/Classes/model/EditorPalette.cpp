#include "model/EditorPalette.h"

namespace tanks::model
{

namespace
{
	constexpr const char* WallObjectType = "Wall";
	constexpr const char* EraseIcon = "erase";
}

bool EditorPalette::FromConfig(const IConfigNode& InNode, const LevelLegend& InLegend, const ConfigLookup& InFindConfig)
{
	Tabs.clear();
	Entries.clear();
	Errors.clear();

	if ( const IConfigNode* const TabsNode = InNode.FindChild("tabs") )
	{
		for ( std::size_t Index = 0; Index < TabsNode->GetItemCount(); ++Index )
		{
			const IConfigNode* const TabNode = TabsNode->GetItem(Index);
			EditorTab Tab;
			if ( !TabNode->TryRead("id", Tab.Id) || Tab.Id.empty() )
			{
				Errors.push_back("a tab without an id");
				continue;
			}
			Tab.NameKey = TabNode->GetValueOr("name", Tab.Id.c_str());
			Tabs.push_back(Tab);
		}
	}

	if ( const IConfigNode* const EntriesNode = InNode.FindChild("entries") )
	{
		for ( std::size_t Index = 0; Index < EntriesNode->GetItemCount(); ++Index )
		{
			EditorPaletteEntry Entry;
			if ( ReadEntry(*EntriesNode->GetItem(Index), InLegend, InFindConfig, Entry) )
			{
				Entries.push_back(Entry);
			}
		}
	}

	return !Tabs.empty() && !Entries.empty();
}

bool EditorPalette::ReadEntry(const IConfigNode& InNode, const LevelLegend& InLegend, const ConfigLookup& InFindConfig, EditorPaletteEntry& OutEntry)
{
	const std::string TabId = InNode.GetValueOr("tab", "");
	const std::string Tile = InNode.GetValueOr("tile", "");
	if ( Tile.size() != 1 )
	{
		Errors.push_back("entry with a bad tile '" + Tile + "'");
		return false;
	}
	if ( !FindTab(TabId, OutEntry.Tab) )
	{
		Errors.push_back("entry '" + Tile + "' has an unknown tab '" + TabId + "'");
		return false;
	}
	const LegendEntry* const Legend = InLegend.Find(Tile.front());
	if ( !Legend )
	{
		Errors.push_back("entry '" + Tile + "' is not in the level legend");
		return false;
	}
	OutEntry.Tile = Tile.front();
	OutEntry.NameKey = InNode.GetValueOr("name", "");

	if ( InNode.GetValueOr("icon", "") == EraseIcon )
	{
		OutEntry.Icon = PaletteIcon::Erase;
		return true;
	}

	if ( !Legend->Config.empty() )
	{
		const IConfigNode* const ObjectNode = InFindConfig ? InFindConfig(Legend->Config) : nullptr;
		if ( !ObjectNode || !ObjectNode->TryRead("texture_path", OutEntry.Frame) )
		{
			Errors.push_back("entry '" + Tile + "': no texture in " + Legend->Config);
			return false;
		}
		OutEntry.MapScale = ObjectNode->GetValueOr("scale", 1.0f);
		OutEntry.Icon = ObjectNode->GetValueOr("object_type", "") == WallObjectType && ObjectNode->GetValueOr("tileset_frame", true) ? PaletteIcon::TilesetFrame : PaletteIcon::Frame;
		return true;
	}

	if ( Legend->Frame.empty() )
	{
		Errors.push_back("entry '" + Tile + "' has no config or frame in the legend");
		return false;
	}
	OutEntry.Frame = Legend->Frame;
	OutEntry.MapScale = Legend->MapScale;
	return true;
}

bool EditorPalette::FindTab(const std::string& InId, std::size_t& OutIndex) const
{
	for ( std::size_t Index = 0; Index < Tabs.size(); ++Index )
	{
		if ( Tabs[Index].Id == InId )
		{
			OutIndex = Index;
			return true;
		}
	}
	return false;
}

std::vector<std::size_t> EditorPalette::GetEntriesOfTab(const std::size_t InTab) const
{
	std::vector<std::size_t> Result;
	for ( std::size_t Index = 0; Index < Entries.size(); ++Index )
	{
		if ( Entries[Index].Tab == InTab )
		{
			Result.push_back(Index);
		}
	}
	return Result;
}

const EditorPaletteEntry* EditorPalette::FindByTile(const char InTile) const
{
	for ( const EditorPaletteEntry& Entry : Entries )
	{
		if ( Entry.Tile == InTile )
		{
			return &Entry;
		}
	}
	return nullptr;
}

} // namespace tanks::model
