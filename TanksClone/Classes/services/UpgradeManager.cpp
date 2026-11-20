#include "services/UpgradeManager.h"

#include "core/ConfigReader.h"
#include "core/DataConverter.h"
#include "core/GameConstants.h"
#include <cmath>
#include <string_view>
#include "services/ProfileManager.h"
#include "game/objects/Tank.h"

namespace tanks
{

UpgradeManager& UpgradeManager::GetInstance()
{
	static UpgradeManager Instance;
	return Instance;
}

void UpgradeManager::Init()
{
	Upgrades.clear();

	const cocos2d::ValueMap& ConfigData = ConfigReader::GetInstance().GetObjectData(UpgradesConfig).Values;
	auto ListIt = ConfigData.find("upgrades");
	if ( ListIt == ConfigData.end() || ListIt->second.getType() != cocos2d::Value::Type::VECTOR )
	{
		return;
	}

	for ( const auto& Item : ListIt->second.asValueVector() )
	{
		if ( Item.getType() != cocos2d::Value::Type::MAP )
		{
			continue;
		}
		const cocos2d::ValueMap& Map = Item.asValueMap();

		UpgradeDef CurrentDef;
		CurrentDef.Id = DataConverter::GetString(Map, "id", "");
		CurrentDef.NameKey = DataConverter::GetString(Map, "name", CurrentDef.Id);
		CurrentDef.MaxLevel = DataConverter::GetInt(Map, "max_level", 1);
		CurrentDef.BaseCost = DataConverter::GetInt(Map, "base_cost", 0);
		CurrentDef.CostStep = DataConverter::GetInt(Map, "cost_step", 0);
		auto ModifiersIt = Map.find("modifiers");
		if ( ModifiersIt != Map.end() )
		{
			CurrentDef.Modifiers = ParseStatModifiers(ModifiersIt->second);
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
	const auto& Levels = ProfileManager::GetInstance().Get().Upgrades;
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
	return Cost >= 0 && ProfileManager::GetInstance().Get().Coins >= Cost;
}

bool UpgradeManager::Buy(const std::string& InId)
{
	if ( !CanBuy(InId) )
	{
		return false;
	}

	auto& CurrentProfile = ProfileManager::GetInstance().Get();
	CurrentProfile.Coins -= GetNextCost(InId);
	CurrentProfile.Upgrades[InId] = GetLevel(InId) + 1;
	ProfileManager::GetInstance().Save();
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