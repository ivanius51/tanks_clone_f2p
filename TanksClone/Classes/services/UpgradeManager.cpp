#include "services/UpgradeManager.h"

#include <cmath>

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/components/StatsLoader.h"
#include "game/objects/Tank.h"
#include "services/ProfileManager.h"

namespace tanks
{

void UpgradeManager::Init()
{
	Upgrades.clear();

	const model::IConfigNode* const List = GameContext::Get().GetManager<ConfigReader>().GetConfig(UpgradesConfig).FindChild("upgrades");
	if ( !List )
	{
		return;
	}

	for ( std::size_t Index = 0; Index < List->GetItemCount(); ++Index )
	{
		const model::IConfigNode& Item = *List->GetItem(Index);

		UpgradeDef CurrentDef;
		CurrentDef.Id = Item.GetValueOr("id", "");
		CurrentDef.NameKey = Item.GetValueOr("name", CurrentDef.Id.c_str());
		CurrentDef.MaxLevel = Item.GetValueOr("max_level", 1);
		CurrentDef.BaseCost = Item.GetValueOr("base_cost", 0);
		CurrentDef.CostStep = Item.GetValueOr("cost_step", 0);
		if ( const model::IConfigNode* const Modifiers = Item.FindChild("modifiers") )
		{
			CurrentDef.Modifiers = StatsLoader::ReadModifiers(*Modifiers);
		}
		if ( !CurrentDef.Id.empty() )
		{
			Upgrades.push_back(CurrentDef);
		}
	}
}

const std::vector<UpgradeDef>& UpgradeManager::GetUpgrades() const
{
	return Upgrades;
}

const UpgradeDef* UpgradeManager::Find(std::string_view InId) const
{
	for ( const auto& Upgrade : Upgrades )
	{
		if ( Upgrade.Id == InId )
		{
			return &Upgrade;
		}
	}
	return nullptr;
}

int UpgradeManager::GetLevel(const std::string& InId) const
{
	const auto& Levels = GameContext::Get().GetManager<ProfileManager>().Get().Upgrades;
	auto LevelIt = Levels.find(InId);
	return LevelIt != Levels.end() ? LevelIt->second : 0;
}

int UpgradeManager::GetNextCost(const std::string& InId) const
{
	const UpgradeDef* CurrentDef = Find(InId);
	const int CurrentLevel = GetLevel(InId);
	if ( !CurrentDef || CurrentLevel >= CurrentDef->MaxLevel )
	{
		return -1;
	}
	return CurrentDef->BaseCost + CurrentDef->CostStep * CurrentLevel;
}

bool UpgradeManager::CanBuy(const std::string& InId) const
{
	const int Cost = GetNextCost(InId);
	return Cost >= 0 && GameContext::Get().GetManager<ProfileManager>().Get().Coins >= Cost;
}

bool UpgradeManager::Buy(const std::string& InId)
{
	if ( !CanBuy(InId) )
	{
		return false;
	}

	auto& CurrentProfile = GameContext::Get().GetManager<ProfileManager>().Get();
	CurrentProfile.Coins -= GetNextCost(InId);
	CurrentProfile.Upgrades[InId] = GetLevel(InId) + 1;
	GameContext::Get().GetManager<ProfileManager>().Save();
	return true;
}

void UpgradeManager::ApplyTo(Tank& InTank) const
{
	for ( const auto& Upgrade : Upgrades )
	{
		const int CurrentLevel = GetLevel(Upgrade.Id);
		if ( CurrentLevel <= 0 )
		{
			continue;
		}

		// One buff per upgrade, with the effect of one level repeated level times.
		BuffDef Buff;
		Buff.Id = "upgrade_" + Upgrade.Id;
		Buff.NameKey = Upgrade.NameKey;
		Buff.Duration = -1.0f;
		for ( const auto& Modifier : Upgrade.Modifiers )
		{
			StatModifier Total = Modifier;
			Total.Add = Modifier.Add * CurrentLevel;
			Total.Mul = std::pow(Modifier.Mul, static_cast<float>(CurrentLevel));
			Buff.Modifiers.push_back(Total);
		}
		InTank.GetBuffs().Add(Buff);
	}

	InTank.RefreshStats();
	InTank.RestoreFull();
}

} // namespace tanks
