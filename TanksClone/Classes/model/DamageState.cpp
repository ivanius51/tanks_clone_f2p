#include "model/DamageState.h"

#include <algorithm>

namespace tanks::model
{

SideHitSettings SideHitSettings::FromConfig(const IConfigNode& InNode)
{
	SideHitSettings Result;
	Result.SlowPerHit = InNode.GetValueOr("slow_per_hit", Result.SlowPerHit);
	Result.MaxSlow = std::clamp(InNode.GetValueOr("max_slow", Result.MaxSlow), 0.0f, 1.0f);
	Result.SlowTime = InNode.GetValueOr("slow_time", Result.SlowTime);
	Result.StopShare = InNode.GetValueOr("stop_share", Result.StopShare);
	Result.StopTime = InNode.GetValueOr("stop_time", Result.StopTime);
	return Result;
}

DamageState::DamageState(const SideHitSettings& InSettings)
	: Settings(InSettings)
{
}

void DamageState::OnSideHit(const int InHealthLost, const int InMaxHealth)
{
	++Stacks;
	SlowTimer = Settings.SlowTime;
	SideLoss += std::max(0, InHealthLost);
	if ( InMaxHealth > 0 && SideLoss > Settings.StopShare * InMaxHealth )
	{
		StopTimer = Settings.StopTime;
		SideLoss = 0;
	}
}

bool DamageState::Update(const float InDelta)
{
	const float Before = GetSpeedScale();
	StopTimer = std::max(0.0f, StopTimer - InDelta);
	SlowTimer = std::max(0.0f, SlowTimer - InDelta);
	if ( SlowTimer <= 0.0f )
	{
		Stacks = 0;
	}
	return GetSpeedScale() != Before;
}

float DamageState::GetSpeedScale() const
{
	if ( IsImmobilized() )
	{
		return 0.0f;
	}
	return 1.0f - std::min(Settings.MaxSlow, Stacks * Settings.SlowPerHit);
}

bool DamageState::IsImmobilized() const
{
	return StopTimer > 0.0f;
}

int DamageState::GetStacks() const
{
	return Stacks;
}

} // namespace tanks::model
