#pragma once

#include <string>
#include <vector>
#include "cocos2d.h"

namespace tanks
{

/** One gun of a tank, from the "weapons" list of its config. */
struct WeaponDef
{
	std::string Id;
	std::string NameKey;      // string id for the HUD
	std::string Icon;         // sprite frame of the HUD slot
	std::string BulletConfig; // config of the bullets it fires
	float ReloadTime = 0.0f;  // seconds; 0 = the tank's "ReloadTime" stat (upgrades and buffs apply)
	int Damage = 0;           // 0 = the tank's "Damage" stat
	bool bUsesAmmo = true;     // false = fires without shells
};

// Reads "weapons": [{"id", "name", "icon", "bullet", "reload_time", "damage", "uses_ammo"}].
// Without the list the tank has one cannon that fires InDefaultBullet.
std::vector<WeaponDef> ReadWeapons(const cocos2d::ValueMap& InData, const std::string& InDefaultBullet);

} // namespace tanks
