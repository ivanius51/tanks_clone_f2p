#include "model/DropRoller.h"

#include <algorithm>

#include "model/WeightedTable.h"
#include "templates/TContainers.h"

namespace game::model
{

void DropRoller::Load(const IConfigNode& InNode)
{
	Tables.Clear();
	Misses.Clear();
	const IConfigNode* const List = InNode.FindChild("tables");
	if ( !List )
	{
		return;
	}
	Tables.ReadList(*List, [](const IConfigNode& Item, DropTable& Table)
	{
		if ( !Item.TryRead("id", Table.Id) )
		{
			return false;
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
				Table.Entries.PushBack(Entry);
			}
		}
		return true;
	});
}

void DropRoller::SetTables(const TArray<DropTable>& InTables)
{
	Tables.SetAll(InTables);
	Misses.Clear();
}

void DropRoller::Reset()
{
	Misses.Clear();
}

std::optional<std::string> DropRoller::Roll(const std::string& InTableId, Random& InRandom, const float InChanceScale)
{
	const DropTable* const Table = Tables.Find(InTableId);
	if ( !Table || Table->Entries.IsEmpty() )
	{
		return std::nullopt;
	}
	if ( !InRandom.Chance(GetCurrentChance(InTableId, InChanceScale)) )
	{
		++Misses[InTableId];
		return std::nullopt;
	}
	Misses[InTableId] = 0;

	WeightedTable<std::string> Pickups;
	for ( const DropEntry& Entry : Table->Entries )
	{
		Pickups.Add(Entry.Pickup, Entry.Weight);
	}
	const std::string* const Picked = Pickups.Pick(InRandom);
	return Picked ? std::optional<std::string>(*Picked) : std::nullopt;
}

float DropRoller::GetCurrentChance(const std::string& InTableId, const float InChanceScale) const
{
	const DropTable* const Table = Tables.Find(InTableId);
	if ( !Table )
	{
		return 0.0f;
	}
	auto* const Found = Misses.Find(InTableId);
	const int MissCount = Found ? *Found : 0;
	return std::clamp((Table->Chance + Table->PityStep * MissCount) * InChanceScale, 0.0f, 1.0f);
}

} // namespace game::model
