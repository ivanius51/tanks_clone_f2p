#include "services/UpgradeManager.h"

#include <cmath>

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "game/components/StatsLoader.h"
#include "game/objects/Tank.h"
#include "services/ProfileManager.h"
#include "services/ResourceManager.h"

namespace tanks
{

void UpgradeManager::Init()
{
	Upgrades.clear();

	const model::IConfigNode* const List = GameContext::Get().GetManager<ConfigReader>().GetConfig(GameContext::Get().GetManager<ResourceManager>().ResolvePath(ids::configs::Upgrades, model::ResourceKind::Config)).FindChild("upgrades");
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
		CurrentDef.AutoFireWeapon = Item.GetValueOr("auto_fire_weapon", "");
		CurrentDef.UnlockWeapon = Item.GetValueOr("unlock_weapon", "");
		CurrentDef.Weapon = Item.GetValueOr("weapon", "");
		CurrentDef.WeaponDamage = Item.GetValueOr("weapon_damage", 0);
		CurrentDef.WeaponReload = Item.GetValueOr("weapon_reload", 1.0f);
		CurrentDef.Requires = Item.GetValueOr("requires", "");
		CurrentDef.RequiresLevel = Item.GetValueOr("requires_level", 1);
		CurrentDef.Branch = Item.GetValueOr("branch", "");
		CurrentDef.Step = Item.GetValueOr("step", 0);
		CurrentDef.Icon = Item.GetValueOr("icon", "");
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
	return Cost >= 0 && IsOpen(InId) && GameContext::Get().GetManager<ProfileManager>().Get().Coins >= Cost;
}

bool UpgradeManager::IsOpen(const std::string& InId) const
{
	const UpgradeDef* const CurrentDef = Find(InId);
	return CurrentDef && (CurrentDef->Requires.empty() || GetLevel(CurrentDef->Requires) >= CurrentDef->RequiresLevel);
}

const UpgradeDef* UpgradeManager::GetRequirement(const std::string& InId) const
{
	const UpgradeDef* const CurrentDef = Find(InId);
	return CurrentDef && !CurrentDef->Requires.empty() ? Find(CurrentDef->Requires) : nullptr;
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
		if ( !Upgrade.UnlockWeapon.empty() )
		{
			InTank.UnlockWeapon(Upgrade.UnlockWeapon);
		}
		if ( !Upgrade.Weapon.empty() )
		{
			InTank.ImproveWeapon(Upgrade.Weapon, Upgrade.WeaponDamage * CurrentLevel, std::pow(Upgrade.WeaponReload, static_cast<float>(CurrentLevel)));
		}
		if ( !Upgrade.AutoFireWeapon.empty() )
		{
			InTank.EnableAutoFire(Upgrade.AutoFireWeapon);
		}
	}

	InTank.RefreshStats();
	InTank.RestoreFull();
}

} // namespace tanks
