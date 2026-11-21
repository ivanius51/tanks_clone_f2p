#include "model/DropRoller.h"

#include <algorithm>

namespace tanks::model
{

void DropRoller::Load(const IConfigNode& InNode)
{
	Tables.clear();
	Misses.clear();
	const IConfigNode* const List = InNode.FindChild("tables");
	for ( std::size_t Index = 0; List && Index < List->GetItemCount(); ++Index )
	{
		const IConfigNode& Item = *List->GetItem(Index);
		DropTable Table;
		if ( !Item.TryRead("id", Table.Id) )
		{
			continue;
		}
		Table.Chance = Item.GetValueOr("chance", Table.Chance);
		Table.PityStep = Item.GetValueOr("pity_step", Table.PityStep);
		const IConfigNode* const Entries = Item.FindChild("entries");
		for ( std::size_t EntryIndex = 0; Entries && EntryIndex < Entries->GetItemCount(); ++EntryIndex )
		{
			const IConfigNode& EntryNode = *Entries->GetItem(EntryIndex);
			DropEntry Entry;
			Entry.Weight = EntryNode.GetValueOr("weight", Entry.Weight);
			if ( EntryNode.TryRead("pickup", Entry.Pickup) && Entry.Weight > 0 )
			{
				Table.Entries.push_back(Entry);
			}
		}
		Tables.push_back(Table);
	}
}

void DropRoller::SetTables(const std::vector<DropTable>& InTables)
{
	Tables = InTables;
	Misses.clear();
}

void DropRoller::Reset()
{
	Misses.clear();
}

std::optional<std::string> DropRoller::Roll(const std::string& InTableId, Random& InRandom, const float InChanceScale)
{
	const DropTable* const Table = Find(InTableId);
	if ( !Table || Table->Entries.empty() )
	{
		return std::nullopt;
	}
	if ( !InRandom.Chance(GetCurrentChance(InTableId, InChanceScale)) )
	{
		++Misses[InTableId];
		return std::nullopt;
	}
	Misses[InTableId] = 0;

	int TotalWeight = 0;
	for ( const DropEntry& Entry : Table->Entries )
	{
		TotalWeight += Entry.Weight;
	}
	int Pick = InRandom.Int(1, TotalWeight);
	for ( const DropEntry& Entry : Table->Entries )
	{
		Pick -= Entry.Weight;
		if ( Pick <= 0 )
		{
			return Entry.Pickup;
		}
	}
	return Table->Entries.back().Pickup;
}

float DropRoller::GetCurrentChance(const std::string& InTableId, const float InChanceScale) const
{
	const DropTable* const Table = Find(InTableId);
	if ( !Table )
	{
		return 0.0f;
	}
	const auto Found = Misses.find(InTableId);
	const int MissCount = Found != Misses.end() ? Found->second : 0;
	return std::clamp((Table->Chance + Table->PityStep * MissCount) * InChanceScale, 0.0f, 1.0f);
}

const DropTable* DropRoller::Find(const std::string& InTableId) const
{
	for ( const DropTable& Table : Tables )
	{
		if ( Table.Id == InTableId )
		{
			return &Table;
		}
	}
	return nullptr;
}

} // namespace tanks::model
