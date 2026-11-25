#include "model/DeathPenalty.h"

#include <algorithm>

namespace game::model
{

void DeathPenalty::Load(const IConfigNode& InNode)
{
	HealthLossPerDeath = InNode.GetValueOr("health_loss_per_death", HealthLossPerDeath);
	AmmoLossPerDeath = InNode.GetValueOr("ammo_loss_per_death", AmmoLossPerDeath);
	MinShare = InNode.GetValueOr("min_share", MinShare);
	RepairCostPerDeath = InNode.GetValueOr("repair_cost_per_death", RepairCostPerDeath);
}

void DeathPenalty::SetValues(const float InHealthLossPerDeath, const float InAmmoLossPerDeath, const float InMinShare, const int InRepairCostPerDeath)
{
	HealthLossPerDeath = InHealthLossPerDeath;
	AmmoLossPerDeath = InAmmoLossPerDeath;
	MinShare = InMinShare;
	RepairCostPerDeath = InRepairCostPerDeath;
}

float DeathPenalty::GetHealthShare(const int InDeaths) const
{
	return GetShare(HealthLossPerDeath, InDeaths);
}

float DeathPenalty::GetAmmoShare(const int InDeaths) const
{
	return GetShare(AmmoLossPerDeath, InDeaths);
}

int DeathPenalty::GetRepairCost(const int InDeaths) const
{
	return std::max(InDeaths, 0) * RepairCostPerDeath;
}

float DeathPenalty::GetShare(const float InLossPerDeath, const int InDeaths) const
{
	return std::clamp(1.0f - InLossPerDeath * std::max(InDeaths, 0), std::min(MinShare, 1.0f), 1.0f);
}

} // namespace game::model
