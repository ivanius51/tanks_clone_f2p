#pragma once

#include "model/ConfigNode.h"

namespace game::model
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

	float GetHealthShare(int InDeaths) const;
	float GetAmmoShare(int InDeaths) const;
	int GetRepairCost(int InDeaths) const;

private:
	float GetShare(float InLossPerDeath, int InDeaths) const;

	float HealthLossPerDeath = 0.15f;
	float AmmoLossPerDeath = 0.15f;
	float MinShare = 0.4f;
	int RepairCostPerDeath = 25;
};
} // namespace game::model
