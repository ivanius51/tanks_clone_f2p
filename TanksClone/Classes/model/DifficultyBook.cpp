#include "model/DifficultyBook.h"

#include <algorithm>
#include <cmath>

namespace tanks::model
{

void DifficultyBook::Load(const IConfigNode& InNode)
{
	Modes.Clear();
	DefaultId = InNode.GetValueOr("default", "");
	const IConfigNode* const List = InNode.FindChild("modes");
	if ( !List )
	{
		return;
	}
	Modes.ReadList(*List, [](const IConfigNode& Item, DifficultyDef& Mode)
	{
		if ( !Item.TryRead("id", Mode.Id) )
		{
			return false;
		}
		Mode.NameKey = Item.GetValueOr("name", Mode.Id.c_str());
		Mode.EnemyHealth = Item.GetValueOr("enemy_health", Mode.EnemyHealth);
		Mode.EnemyDamage = Item.GetValueOr("enemy_damage", Mode.EnemyDamage);
		Mode.SpawnInterval = Item.GetValueOr("spawn_interval", Mode.SpawnInterval);
		Mode.Coins = Item.GetValueOr("coins", Mode.Coins);
		Mode.Drops = Item.GetValueOr("drops", Mode.Drops);
		Mode.BulletSpeed = Item.GetValueOr("bullet_speed", Mode.BulletSpeed);
		Mode.ReloadTime = Item.GetValueOr("reload_time", Mode.ReloadTime);
		Mode.SpawnCount = Item.GetValueOr("spawn_count", Mode.SpawnCount);
		Mode.AbilityCooldown = Item.GetValueOr("ability_cooldown", Mode.AbilityCooldown);
		Mode.AiThinkTime = Item.GetValueOr("ai_think_time", Mode.AiThinkTime);
		Mode.AiFireRate = Item.GetValueOr("ai_fire_rate", Mode.AiFireRate);
		Mode.AiAimTolerance = Item.GetValueOr("ai_aim_tolerance", Mode.AiAimTolerance);
		Mode.bFriendlyFire = Item.GetValueOr("friendly_fire", Mode.bFriendlyFire);
		Mode.FriendlyFireShare = std::max(0.0f, Item.GetValueOr("friendly_fire_share", Mode.FriendlyFireShare));
		Mode.bBulletClash = Item.GetValueOr("bullet_clash", Mode.bBulletClash);
		Mode.AiFeatures = AiFeatureSet::ReadSwitches(Item);
		Mode.bDeathPenalty = Item.GetValueOr("death_penalty", Mode.bDeathPenalty);
		return true;
	});
}

void DifficultyBook::SetModes(const std::vector<DifficultyDef>& InModes, const std::string& InDefaultId)
{
	Modes.SetAll(InModes);
	DefaultId = InDefaultId;
}

const std::vector<DifficultyDef>& DifficultyBook::GetModes() const
{
	return Modes.GetAll();
}

const DifficultyDef& DifficultyBook::Find(const std::string& InId) const
{
	if ( const DifficultyDef* const Found = Modes.Find(InId) )
	{
		return *Found;
	}
	if ( const DifficultyDef* const Default = Modes.Find(DefaultId) )
	{
		return *Default;
	}
	return Modes.IsEmpty() ? Neutral : Modes.GetAll().front();
}

std::string DifficultyBook::GetNextId(const std::string& InId) const
{
	const std::vector<DifficultyDef>& All = Modes.GetAll();
	if ( All.empty() )
	{
		return InId;
	}
	for ( std::size_t Index = 0; Index < All.size(); ++Index )
	{
		if ( All[Index].Id == InId )
		{
			return All[(Index + 1) % All.size()].Id;
		}
	}
	return Find(InId).Id;
}

int DifficultyBook::ScaleCount(const int InCount, const float InScale)
{
	return std::max(1, static_cast<int>(std::lround(InCount * std::max(0.0f, InScale))));
}

const std::string& DifficultyBook::GetDefaultId() const
{
	return DefaultId;
}

} // namespace tanks::model
