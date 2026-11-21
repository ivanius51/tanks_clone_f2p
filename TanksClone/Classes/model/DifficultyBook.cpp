#include "model/DifficultyBook.h"

namespace tanks::model
{

void DifficultyBook::Load(const IConfigNode& InNode)
{
	Modes.clear();
	DefaultId = InNode.GetValueOr("default", "");
	const IConfigNode* const List = InNode.FindChild("modes");
	for ( std::size_t Index = 0; List && Index < List->GetItemCount(); ++Index )
	{
		const IConfigNode& Item = *List->GetItem(Index);
		DifficultyDef Mode;
		if ( Item.TryRead("id", Mode.Id) )
		{
			Mode.NameKey = Item.GetValueOr("name", Mode.Id.c_str());
			Mode.EnemyHealth = Item.GetValueOr("enemy_health", Mode.EnemyHealth);
			Mode.EnemyDamage = Item.GetValueOr("enemy_damage", Mode.EnemyDamage);
			Mode.SpawnInterval = Item.GetValueOr("spawn_interval", Mode.SpawnInterval);
			Mode.Coins = Item.GetValueOr("coins", Mode.Coins);
			Mode.Drops = Item.GetValueOr("drops", Mode.Drops);
			Mode.bDeathPenalty = Item.GetValueOr("death_penalty", Mode.bDeathPenalty);
			Modes.push_back(Mode);
		}
	}
}

void DifficultyBook::SetModes(const std::vector<DifficultyDef>& InModes, const std::string& InDefaultId)
{
	Modes = InModes;
	DefaultId = InDefaultId;
}

const std::vector<DifficultyDef>& DifficultyBook::GetModes() const
{
	return Modes;
}

const DifficultyDef& DifficultyBook::Find(const std::string& InId) const
{
	const DifficultyDef* Fallback = Modes.empty() ? &Neutral : &Modes.front();
	for ( const DifficultyDef& Mode : Modes )
	{
		if ( Mode.Id == InId )
		{
			return Mode;
		}
		if ( Mode.Id == DefaultId )
		{
			Fallback = &Mode;
		}
	}
	return *Fallback;
}

std::string DifficultyBook::GetNextId(const std::string& InId) const
{
	if ( Modes.empty() )
	{
		return InId;
	}
	for ( std::size_t Index = 0; Index < Modes.size(); ++Index )
	{
		if ( Modes[Index].Id == InId )
		{
			return Modes[(Index + 1) % Modes.size()].Id;
		}
	}
	return Find(InId).Id;
}

const std::string& DifficultyBook::GetDefaultId() const
{
	return DefaultId;
}

} // namespace tanks::model
