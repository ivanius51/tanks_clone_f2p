#pragma once

#include <string>
#include <vector>

#include "game/objects/TankDef.h"
#include "model/ConfigNode.h"

namespace tanks
{

/** The only place that knows the layout of the tank-specific part of a tank config; Tank gets a plain TankDef. */
class TankLoader
{
public:
	// Keys: "armor", "reload_time", "max_ammo", "weapons", "abilities", "buffs", "enemy_type", "score", "coins", "ai".
	static TankDef ReadTank(const model::IConfigNode& InNode);
	// "weapons": [{"id", "name", "icon", "bullet", "reload_time", "damage", "uses_ammo"}].
	// Without the list the tank has one cannon that fires InDefaultBullet.
	static std::vector<WeaponDef> ReadWeapons(const model::IConfigNode& InNode, const std::string& InDefaultBullet);
	// "abilities": [{"type": "Buff", "buff": "shield", "cooldown": 8}]. Without the list the tank just shoots.
	static std::vector<AbilityDef> ReadAbilities(const model::IConfigNode& InNode);
};

} // namespace tanks
