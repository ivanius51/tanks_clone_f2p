#include "model/LevelLegend.h"
#include "templates/TContainers.h"

#include <algorithm>

namespace game::model
{

namespace
{
	struct RoleName
	{
		const char* Name;
		TileRole Role;
	};

	constexpr RoleName RoleNames[] = {
		{ "empty", TileRole::Empty }, { "wall", TileRole::Wall }, { "prop", TileRole::Prop }, { "pickup", TileRole::Pickup },
		{ "crate", TileRole::Crate }, { "player", TileRole::Player }, { "spawner", TileRole::Spawner } };

	bool ReadPass(const std::string& InName, CellPass& OutPass)
	{
		if ( InName == "open" )
		{
			OutPass = CellPass::Open;
		}
		else if ( InName == "breakable" )
		{
			OutPass = CellPass::Breakable;
		}
		else if ( InName == "blocked" )
		{
			OutPass = CellPass::Blocked;
		}
		else
		{
			return false;
		}
		return true;
	}
}

bool LevelLegend::FromConfig(const IConfigNode& InNode)
{
	Entries.Clear();
	Grounds.Clear();
	Errors.Clear();
	const bool bRead = ReadEntries(InNode);
	ReadGrounds(InNode);
	return bRead;
}

const GroundEntry* LevelLegend::FindGround(const char InTile) const
{
	for ( const GroundEntry& Ground : Grounds )
	{
		if ( Ground.Tile == InTile )
		{
			return &Ground;
		}
	}
	return nullptr;
}

void LevelLegend::ReadGrounds(const IConfigNode& InNode)
{
	const IConfigNode* const GroundNode = InNode.FindChild("ground");
	if ( !GroundNode )
	{
		return;
	}
	for ( std::size_t Index = 0; Index < GroundNode->GetItemCount(); ++Index )
	{
		const IConfigNode& Item = *GroundNode->GetItem(Index);
		const std::string Tile = Item.GetValueOr("tile", "");
		if ( Tile.size() != 1 || FindGround(Tile[0]) || Find(Tile[0]) )
		{
			Errors.PushBack("ground entry with a bad or repeated tile '" + Tile + "'");
			continue;
		}
		GroundEntry Entry;
		Entry.Tile = Tile[0];
		Entry.NameKey = Item.GetValueOr("name", "");
		Entry.SpeedScale = std::max(0.1f, Item.GetValueOr("speed_scale", 1.0f));
		Entry.bLeavesMarks = Item.GetValueOr("leaves_marks", true);
		TArray<int> Tint;
		Item.TryRead("tint", Tint);
		if ( Tint.Size() == 3 )
		{
			Entry.Tint = Color(static_cast<std::uint8_t>(std::clamp(Tint[0], 0, 255)), static_cast<std::uint8_t>(std::clamp(Tint[1], 0, 255)), static_cast<std::uint8_t>(std::clamp(Tint[2], 0, 255)));
		}
		Grounds.PushBack(Entry);
	}
}

bool LevelLegend::ReadEntries(const IConfigNode& InNode)
{
	const IConfigNode* const TilesNode = InNode.FindChild("tiles");
	if ( !TilesNode )
	{
		Errors.PushBack("no tiles section");
		return false;
	}

	for ( std::size_t Index = 0; Index < TilesNode->GetItemCount(); ++Index )
	{
		const IConfigNode& Item = *TilesNode->GetItem(Index);
		const std::string Tiles = Item.GetValueOr("tile", "");
		LegendEntry Template;
		if ( Tiles.empty() )
		{
			Errors.PushBack("an entry without a tile");
			continue;
		}
		if ( !ReadRole(Item, Tiles, Template) )
		{
			continue;
		}

		for ( const char Tile : Tiles )
		{
			if ( Find(Tile) )
			{
				Errors.PushBack(std::string("tile '") + Tile + "' is listed twice");
				continue;
			}
			LegendEntry Entry = Template;
			Entry.Tile = Tile;
			Entries.PushBack(Entry);
		}
	}
	return !Entries.IsEmpty();
}

bool LevelLegend::ReadRole(const IConfigNode& InNode, const std::string& InTiles, LegendEntry& OutEntry)
{
	const std::string RoleText = InNode.GetValueOr("role", "");
	const auto* const Found = std::find_if(std::begin(RoleNames), std::end(RoleNames), [&RoleText](const RoleName& InName) { return RoleText == InName.Name; });
	if ( Found == std::end(RoleNames) )
	{
		Errors.PushBack("'" + InTiles + "' has an unknown role '" + RoleText + "'");
		return false;
	}
	OutEntry.Role = Found->Role;

	const std::string PassText = InNode.GetValueOr("pass", "open");
	if ( !ReadPass(PassText, OutEntry.Pass) )
	{
		Errors.PushBack("'" + InTiles + "' has an unknown pass '" + PassText + "'");
		return false;
	}

	InNode.TryRead("config", OutEntry.Config);
	InNode.TryRead("frame", OutEntry.Frame);
	OutEntry.MapScale = InNode.GetValueOr("scale", 1.0f);
	OutEntry.bUnique = InNode.GetValueOr("unique", false);
	OutEntry.bIsBase = InNode.GetValueOr("is_base", false);
	OutEntry.bDefaultSpawner = InNode.GetValueOr("default", false);

	const bool bNeedsConfig = OutEntry.Role == TileRole::Wall || OutEntry.Role == TileRole::Prop || OutEntry.Role == TileRole::Pickup;
	if ( bNeedsConfig && OutEntry.Config.empty() )
	{
		Errors.PushBack("'" + InTiles + "' needs a config");
		return false;
	}
	if ( OutEntry.Role == TileRole::Player )
	{
		OutEntry.bUnique = true;
	}
	return true;
}

const LegendEntry* LevelLegend::Find(const char InTile) const
{
	for ( const LegendEntry& Entry : Entries )
	{
		if ( Entry.Tile == InTile )
		{
			return &Entry;
		}
	}
	return nullptr;
}

bool LevelLegend::IsKnown(const char InTile) const
{
	return Find(InTile) != nullptr;
}

bool LevelLegend::IsSpawner(const char InTile) const
{
	const LegendEntry* const Entry = Find(InTile);
	return Entry && Entry->Role == TileRole::Spawner;
}

char LevelLegend::GetDefaultSpawner() const
{
	for ( const LegendEntry& Entry : Entries )
	{
		if ( Entry.bDefaultSpawner )
		{
			return Entry.Tile;
		}
	}
	return 'E';
}

LegendScan LevelLegend::Scan(const TArray<std::string>& InRows) const
{
	LegendScan Result;
	std::string UniqueSeen;
	for ( std::size_t Row = 0; Row < InRows.Size(); ++Row )
	{
		for ( std::size_t Column = 0; Column < InRows[Row].size(); ++Column )
		{
			const char Tile = InRows[Row][Column];
			const LegendEntry* const Entry = Find(Tile);
			if ( !Entry )
			{
				Result.bUnknownTile = true;
				continue;
			}
			const GridCell Cell{ static_cast<int>(Column), static_cast<int>(Row) };
			if ( Entry->bUnique && Entry->Role != TileRole::Player )
			{
				if ( UniqueSeen.find(Tile) != std::string::npos )
				{
					Result.bUniqueRepeated = true;
				}
				UniqueSeen += Tile;
			}
			if ( Entry->Role == TileRole::Player )
			{
				Result.Players.PushBack(Cell);
			}
			else if ( Entry->Role == TileRole::Spawner )
			{
				Result.Spawners.PushBack(Cell);
				Result.SpawnerTiles += Tile;
			}
		}
	}
	return Result;
}

GridPath LevelLegend::BuildPathGrid(const TArray<std::string>& InRows, const int InWidth) const
{
	GridPath Grid(InWidth, static_cast<int>(InRows.Size()));
	for ( int Row = 0; Row < Grid.GetHeight(); ++Row )
	{
		const std::string& Tiles = InRows[Row];
		for ( int Column = 0; Column < std::min(Grid.GetWidth(), static_cast<int>(Tiles.size())); ++Column )
		{
			const LegendEntry* const Entry = Find(Tiles[Column]);
			Grid.Set({ Column, Row }, Entry ? Entry->Pass : CellPass::Open);
		}
	}
	return Grid;
}

} // namespace game::model
