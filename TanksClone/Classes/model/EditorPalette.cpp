#include "model/EditorPalette.h"
#include "templates/TContainers.h"

namespace game::model
{

namespace
{
	constexpr const char* WallObjectType = "Wall";
	constexpr const char* EraseIcon = "erase";
	constexpr const char* FloorLayerName = "floor";
	constexpr const char* PointsLayerName = "points";
	constexpr const char* PointsTabId = "points";
	constexpr char EraseTile = '.';
	constexpr const char* EraseNameKey = "EDITOR_ERASE";
}

bool EditorPalette::FromConfig(const IConfigNode& InNode, const LevelLegend& InLegend, const ConfigLookup& InFindConfig)
{
	Tabs.Clear();
	Entries.Clear();
	PointNames.Clear();
	Errors.Clear();

	if ( const IConfigNode* const TabsNode = InNode.FindChild("tabs") )
	{
		for ( std::size_t Index = 0; Index < TabsNode->GetItemCount(); ++Index )
		{
			const IConfigNode* const TabNode = TabsNode->GetItem(Index);
			EditorTab Tab;
			if ( !TabNode->TryRead("id", Tab.Id) || Tab.Id.empty() )
			{
				Errors.PushBack("a tab without an id");
				continue;
			}
			Tab.NameKey = TabNode->GetValueOr("name", Tab.Id.c_str());
			const std::string LayerName = TabNode->GetValueOr("layer", "");
			Tab.Layer = LayerName == FloorLayerName ? PaletteLayer::Floor : LayerName == PointsLayerName ? PaletteLayer::Points : PaletteLayer::Ground;
			Tabs.PushBack(Tab);
		}
	}

	if ( const IConfigNode* const EntriesNode = InNode.FindChild("entries") )
	{
		for ( std::size_t Index = 0; Index < EntriesNode->GetItemCount(); ++Index )
		{
			EditorPaletteEntry Entry;
			if ( ReadEntry(*EntriesNode->GetItem(Index), InLegend, InFindConfig, Entry) )
			{
				Entries.PushBack(Entry);
			}
		}
	}

	if ( const IConfigNode* const NamesNode = InNode.FindChild("point_names") )
	{
		for ( std::size_t Index = 0; Index < NamesNode->GetItemCount(); ++Index )
		{
			if ( const std::optional<std::string> Name = NamesNode->GetItem(Index)->ReadString(); Name && !Name->empty() )
			{
				PointNames.PushBack(*Name);
			}
		}
	}

	AddPointEntries();
	return !Tabs.IsEmpty() && !Entries.IsEmpty();
}

void EditorPalette::AddPointEntries()
{
	std::size_t PointsTab = 0;
	if ( !FindTab(PointsTabId, PointsTab) || Tabs[PointsTab].Layer != PaletteLayer::Points )
	{
		return;
	}
	for ( const std::string& Name : PointNames )
	{
		EditorPaletteEntry Entry;
		Entry.Tab = PointsTab;
		Entry.NameKey = Name;
		Entry.Icon = PaletteIcon::Point;
		Entries.PushBack(Entry);
	}
	EditorPaletteEntry Remove;
	Remove.Tab = PointsTab;
	Remove.Tile = EraseTile;
	Remove.NameKey = EraseNameKey;
	Remove.Icon = PaletteIcon::Erase;
	Entries.PushBack(Remove);
}

bool EditorPalette::ReadEntry(const IConfigNode& InNode, const LevelLegend& InLegend, const ConfigLookup& InFindConfig, EditorPaletteEntry& OutEntry)
{
	const std::string TabId = InNode.GetValueOr("tab", "");
	const std::string Tile = InNode.GetValueOr("tile", "");
	if ( Tile.size() != 1 )
	{
		Errors.PushBack("entry with a bad tile '" + Tile + "'");
		return false;
	}
	if ( !FindTab(TabId, OutEntry.Tab) )
	{
		Errors.PushBack("entry '" + Tile + "' has an unknown tab '" + TabId + "'");
		return false;
	}
	if ( Tabs[OutEntry.Tab].Layer == PaletteLayer::Floor )
	{
		return ReadFloorEntry(InNode, InLegend, Tile.front(), OutEntry);
	}
	const LegendEntry* const Legend = InLegend.Find(Tile.front());
	if ( !Legend )
	{
		Errors.PushBack("entry '" + Tile + "' is not in the level legend");
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
			Errors.PushBack("entry '" + Tile + "': no texture in " + Legend->Config);
			return false;
		}
		OutEntry.MapScale = ObjectNode->GetValueOr("scale", 1.0f);
		OutEntry.Icon = ObjectNode->GetValueOr("object_type", "") == WallObjectType && ObjectNode->GetValueOr("tileset_frame", true) ? PaletteIcon::TilesetFrame : PaletteIcon::Frame;
		return true;
	}

	if ( Legend->Frame.empty() )
	{
		Errors.PushBack("entry '" + Tile + "' has no config or frame in the legend");
		return false;
	}
	OutEntry.Frame = Legend->Frame;
	OutEntry.MapScale = Legend->MapScale;
	return true;
}

bool EditorPalette::ReadFloorEntry(const IConfigNode& InNode, const LevelLegend& InLegend, const char InTile, EditorPaletteEntry& OutEntry)
{
	OutEntry.Tile = InTile;
	OutEntry.NameKey = InNode.GetValueOr("name", "");
	if ( InTile == EraseTile )
	{
		OutEntry.Icon = PaletteIcon::Erase;
		return true;
	}
	const GroundEntry* const Ground = InLegend.FindGround(InTile);
	if ( !Ground )
	{
		Errors.PushBack(std::string("floor entry '") + InTile + "' is not in the legend ground list");
		return false;
	}
	OutEntry.Icon = PaletteIcon::Swatch;
	OutEntry.Tint = Ground->Tint;
	return true;
}

bool EditorPalette::FindTab(const std::string& InId, std::size_t& OutIndex) const
{
	for ( std::size_t Index = 0; Index < Tabs.Size(); ++Index )
	{
		if ( Tabs[Index].Id == InId )
		{
			OutIndex = Index;
			return true;
		}
	}
	return false;
}

TArray<std::size_t> EditorPalette::GetEntriesOfTab(const std::size_t InTab) const
{
	TArray<std::size_t> Result;
	for ( std::size_t Index = 0; Index < Entries.Size(); ++Index )
	{
		if ( Entries[Index].Tab == InTab )
		{
			Result.PushBack(Index);
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

} // namespace game::model
