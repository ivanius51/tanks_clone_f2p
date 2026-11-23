#pragma once

#include "model/ConfigNode.h"

namespace tanks::model
{

/**
 * What dying on a level costs (configs/balance/death_penalty.json): every death on it starts the next try with a smaller
 * share of health and ammo, down to a floor, until the level is won or the tank is repaired for coins.
 */
class DeathPenalty
{
public:
	void Load(const IConfigNode& InNode);
	void SetValues(float InHealthLossPerDeath, float InAmmoLossPerDeath, float InMinShare, int InRepairCostPerDeath);

	// Share of the maximum the tank starts with, 1 without deaths.
	float GetHealthShare(int InDeaths) const;
	float GetAmmoShare(int InDeaths) const;
	// Coins that clear the deaths; 0 when there is nothing to repair.
	int GetRepairCost(int InDeaths) const;

private:
	float GetShare(float InLossPerDeath, int InDeaths) const;

	float HealthLossPerDeath = 0.15f;
	float AmmoLossPerDeath = 0.15f;
	float MinShare = 0.4f;
	int RepairCostPerDeath = 25;
};

} // namespace tanks::model
