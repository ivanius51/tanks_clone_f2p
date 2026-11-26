#pragma once

#include <string>

#include "game/objects/TankDef.h"
#include "model/ConfigNode.h"
#include "templates/TContainers.h"

namespace game
{
/** The only place that knows the layout of the tank-specific part of a tank config; Tank gets a plain TankDef. */
class TankLoader
{
public:
	static TankDef ReadTank(const model::IConfigNode& InNode);
	static void ReadFlash(const model::IConfigNode& InNode, WeaponDef& OutWeapon);
	static TArray<WeaponDef> ReadWeapons(const model::IConfigNode& InNode, const std::string& InDefaultBullet);
	static TArray<AbilityDef> ReadAbilities(const model::IConfigNode& InNode);
	static TArray<TankPhaseDef> ReadPhases(const model::IConfigNode& InNode);
};
} // namespace game
