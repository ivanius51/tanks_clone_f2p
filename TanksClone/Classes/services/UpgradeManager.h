#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "game/components/Stats.h"

namespace tanks
{

class Tank;

// One thing the player can buy several times, defined in configs/upgrades.json.
// Each bought level applies "modifiers" once more, as a permanent buff on the player's tank.
struct UpgradeDef
{
	std::string Id;
	std::string NameKey; // string id for the title
	int MaxLevel = 1;
	int BaseCost = 0;    // price of the first level
	int CostStep = 0;    // every next level costs this much more
	std::vector<StatModifier> Modifiers; // effect of ONE level
	std::string AutoFireWeapon; // id of the weapon that fires while the trigger is held once bought
	std::string UnlockWeapon;   // id of a "locked" weapon of the tank that becomes usable once bought
};

// The shop: knows what can be bought, what it costs and gives the bought upgrades to the tank.
class UpgradeManager
{
public:
	UpgradeManager() = default;
	~UpgradeManager() = default;
	UpgradeManager(const UpgradeManager&) = delete;
	UpgradeManager& operator=(const UpgradeManager&) = delete;

	// Call after the configs are loaded.
	void Init();

	const std::vector<UpgradeDef>& GetUpgrades() const;
	int GetLevel(const std::string& InId) const;
	// Price of the next level, or -1 if the upgrade is at its maximum.
	int GetNextCost(const std::string& InId) const;
	bool CanBuy(const std::string& InId) const;
	// Spends the coins, raises the level and saves the profile.
	bool Buy(const std::string& InId);

	// Gives the tank everything that was bought and refills it.
	void ApplyTo(Tank& InTank) const;

private:
	const UpgradeDef* Find(std::string_view InId) const;

	std::vector<UpgradeDef> Upgrades;
};

} // namespace tanks
