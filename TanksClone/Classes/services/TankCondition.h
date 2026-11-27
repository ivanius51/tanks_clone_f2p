#pragma once

#include "model/DeathPenalty.h"

namespace game
{

/**
 * The state of the player's tank before a campaign level: deaths there since it was last won or repaired
 * (counted only when the difficulty has the death penalty) and what that costs in health, ammo and coins.
 */
class TankCondition
{
public:
	static TankCondition ForLevel(int InLevel);
	static model::DeathPenalty LoadPenalty();

	bool IsDamaged() const;
	int GetDeaths() const;
	float GetHealthShare() const;
	float GetAmmoShare() const;
	int GetRepairCost() const;
	bool CanAffordRepair() const;
	// Pays the coins, clears the deaths and saves the profile; false when there is nothing to repair or not enough coins.
	bool Repair();

private:
	TankCondition(int InLevel, int InDeaths, const model::DeathPenalty& InPenalty);

	int Level = 0;
	int Deaths = 0;
	model::DeathPenalty Penalty;
};

} // namespace game
