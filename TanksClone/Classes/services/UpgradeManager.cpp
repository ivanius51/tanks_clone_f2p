#include "services/ConfigRefs.h"
#include "model/Log.h"
#include "services/UpgradeManager.h"

#include <cmath>

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "game/components/StatsLoader.h"
#include "game/objects/Tank.h"
#include "services/AchievementManager.h"
#include "services/ProfileManager.h"
#include "services/ResourceManager.h"
#include "templates/TContainers.h"

namespace game
{

void UpgradeManager::Init()
{
	Upgrades.Clear();

	const model::IConfigNode* const List = config_refs::FindChild(ids::configs::Upgrades, "upgrades");
	if ( !List )
	{
		return;
	}

	const model::IConfigNode* const TreeNodes = config_refs::FindChild(ids::configs::UpgradeTree, "nodes");

	for ( std::size_t Index = 0; Index < List->GetItemCount(); ++Index )
	{
		const model::IConfigNode& Item = *List->GetItem(Index);

		UpgradeDef CurrentDef;
		CurrentDef.Id = FPersistentString{ Item.GetValueOr("id", std::string()) };
		CurrentDef.NameKey = FPersistentString{ Item.GetValueOr("name", std::string(CurrentDef.Id.View())) };
		CurrentDef.MaxLevel = Item.GetValueOr("max_level", 1);
		CurrentDef.BaseCost = Item.GetValueOr("base_cost", 0);
		CurrentDef.CostStep = Item.GetValueOr("cost_step", 0);
		CurrentDef.AutoFireWeapon = FPersistentString{ Item.GetValueOr("auto_fire_weapon", std::string()) };
		CurrentDef.UnlockWeapon = FPersistentString{ Item.GetValueOr("unlock_weapon", std::string()) };
		CurrentDef.Weapon = FPersistentString{ Item.GetValueOr("weapon", std::string()) };
		CurrentDef.WeaponDamage = Item.GetValueOr("weapon_damage", 0);
		CurrentDef.WeaponReload = Item.GetValueOr("weapon_reload", 1.0f);
		CurrentDef.Requires = FPersistentString{ Item.GetValueOr("requires", std::string()) };
		CurrentDef.RequiresLevel = Item.GetValueOr("requires_level", 1);
		const model::IConfigNode* const TreeNode = TreeNodes ? TreeNodes->FindChild(CurrentDef.Id.View()) : nullptr;
		if ( !TreeNode )
		{
			model::Log::Error("UpgradeManager: no tree node for upgrade %s in the upgrade tree config", std::string(CurrentDef.Id.View()).c_str());
		}
		else
		{
			CurrentDef.Branch = FPersistentString{ TreeNode->GetValueOr("branch", std::string()) };
			CurrentDef.Step = TreeNode->GetValueOr("step", 0);
			CurrentDef.Icon = FPersistentString{ TreeNode->GetValueOr("icon", std::string()) };
		}
		CurrentDef.HiddenUntil = FPersistentString{ Item.GetValueOr("hidden_until", std::string()) };
		if ( const model::IConfigNode* const Modifiers = Item.FindChild("modifiers") )
		{
			CurrentDef.Modifiers = StatsLoader::ReadModifiers(*Modifiers);
		}
		if ( !CurrentDef.Id.IsEmpty() )
		{
			Upgrades.Add(CurrentDef);
		}
	}
}

const TArray<UpgradeDef>& UpgradeManager::GetUpgrades() const
{
	return Upgrades.GetAll();
}

const UpgradeDef* UpgradeManager::Find(std::string_view InId) const
{
	return Upgrades.Find(InId);
}

int UpgradeManager::GetLevel(const std::string_view InId) const
{
	const auto& Levels = GameContext::Get<ProfileManager>().Get().Upgrades;
	auto* const LevelIt = Levels.Find(InId);
	return LevelIt ? *LevelIt : 0;
}

int UpgradeManager::GetNextCost(const std::string_view InId) const
{
	const UpgradeDef* CurrentDef = Find(InId);
	const int CurrentLevel = GetLevel(InId);
	if ( !CurrentDef || CurrentLevel >= CurrentDef->MaxLevel )
	{
		return -1;
	}
	return CurrentDef->BaseCost + CurrentDef->CostStep * CurrentLevel;
}

bool UpgradeManager::CanBuy(const std::string_view InId) const
{
	const int Cost = GetNextCost(InId);
	return Cost >= 0 && IsOpen(InId) && !IsHidden(InId) && GameContext::Get<ProfileManager>().Get().Coins >= Cost;
}

bool UpgradeManager::IsOpen(const std::string_view InId) const
{
	const UpgradeDef* const CurrentDef = Find(InId);
	return CurrentDef && (CurrentDef->Requires.IsEmpty() || GetLevel(CurrentDef->Requires.View()) >= CurrentDef->RequiresLevel);
}

bool UpgradeManager::IsHidden(const std::string_view InId) const
{
	const UpgradeDef* const CurrentDef = Find(InId);
	return CurrentDef && !CurrentDef->HiddenUntil.IsEmpty() && !GameContext::Get<AchievementManager>().IsUnlocked(std::string(CurrentDef->HiddenUntil.View()));
}

const UpgradeDef* UpgradeManager::GetRequirement(const std::string_view InId) const
{
	const UpgradeDef* const CurrentDef = Find(InId);
	return CurrentDef && !CurrentDef->Requires.IsEmpty() ? Find(CurrentDef->Requires.View()) : nullptr;
}

bool UpgradeManager::Buy(const std::string_view InId)
{
	if ( !CanBuy(InId) )
	{
		return false;
	}

	auto& CurrentProfile = GameContext::Get<ProfileManager>().Get();
	CurrentProfile.Coins -= GetNextCost(InId);
	CurrentProfile.Upgrades[InId] = GetLevel(InId) + 1;
	GameContext::Get<ProfileManager>().Save();
	return true;
}

void UpgradeManager::ApplyTo(Tank& InTank) const
{
	for ( const auto& Upgrade : Upgrades.GetAll() )
	{
		const int CurrentLevel = GetLevel(Upgrade.Id.View());
		if ( CurrentLevel <= 0 )
		{
			continue;
		}

		// One buff per upgrade, with the effect of one level repeated level times.
		BuffDef Buff;
		Buff.Id = FPersistentString{ "upgrade_" + std::string(Upgrade.Id.View()) };
		Buff.NameKey = FPersistentString{ Upgrade.NameKey };
		Buff.Duration = -1.0f;
		for ( const auto& Modifier : Upgrade.Modifiers )
		{
			StatModifier Total = Modifier;
			Total.Add = Modifier.Add * CurrentLevel;
			Total.Mul = std::pow(Modifier.Mul, static_cast<float>(CurrentLevel));
			Buff.Modifiers.PushBack(Total);
		}
		InTank.GetBuffs().Add(Buff);
		if ( !Upgrade.UnlockWeapon.IsEmpty() )
		{
			InTank.UnlockWeapon(Upgrade.UnlockWeapon.View());
		}
		if ( !Upgrade.Weapon.IsEmpty() )
		{
			InTank.ImproveWeapon(Upgrade.Weapon.View(), Upgrade.WeaponDamage * CurrentLevel, std::pow(Upgrade.WeaponReload, static_cast<float>(CurrentLevel)));
		}
		if ( !Upgrade.AutoFireWeapon.IsEmpty() )
		{
			InTank.EnableAutoFire(Upgrade.AutoFireWeapon.View());
		}
	}

	InTank.RefreshStats();
	InTank.RestoreFull();
}

} // namespace game
