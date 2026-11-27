#include "services/TankCondition.h"

#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "services/ConfigRefs.h"
#include "services/DifficultyManager.h"
#include "services/ProfileManager.h"

namespace game
{

TankCondition::TankCondition(const int InLevel, const int InDeaths, const model::DeathPenalty& InPenalty)
	: Level(InLevel)
	, Deaths(InDeaths)
	, Penalty(InPenalty)
{
}

model::DeathPenalty TankCondition::LoadPenalty()
{
	model::DeathPenalty Result;
	if ( const model::IConfigNode* const Node = config_refs::FindChild(ids::configs::DeathPenalty, "death_penalty") )
	{
		Result.Load(*Node);
	}
	return Result;
}

TankCondition TankCondition::ForLevel(const int InLevel)
{
	int Deaths = 0;
	if ( GameContext::Get<DifficultyManager>().GetCurrent().bDeathPenalty )
	{
		const auto& AllDeaths = GameContext::Get<ProfileManager>().Get().LevelDeaths;
		const auto* const Found = AllDeaths.Find(InLevel);
		Deaths = Found ? *Found : 0;
	}
	return TankCondition(InLevel, Deaths, LoadPenalty());
}

bool TankCondition::IsDamaged() const
{
	return Deaths > 0;
}

int TankCondition::GetDeaths() const
{
	return Deaths;
}

float TankCondition::GetHealthShare() const
{
	return Penalty.GetHealthShare(Deaths);
}

float TankCondition::GetAmmoShare() const
{
	return Penalty.GetAmmoShare(Deaths);
}

int TankCondition::GetRepairCost() const
{
	return Penalty.GetRepairCost(Deaths);
}

bool TankCondition::CanAffordRepair() const
{
	return IsDamaged() && GameContext::Get<ProfileManager>().Get().Coins >= GetRepairCost();
}

bool TankCondition::Repair()
{
	if ( !CanAffordRepair() )
	{
		return false;
	}
	ProfileManager& Profiles = GameContext::Get<ProfileManager>();
	Profiles.Get().Coins -= GetRepairCost();
	Profiles.Get().LevelDeaths.Remove(Level);
	Profiles.Save();
	Deaths = 0;
	return true;
}

} // namespace game
