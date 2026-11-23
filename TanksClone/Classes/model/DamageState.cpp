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

DamageState::DamageState(const SideHitSettings& InSettings, const SimulationClock& InClock)
	: Settings(InSettings)
	, Clock(&InClock)
{
}

double DamageState::Now() const
{
	return Clock ? Clock->GetNow() : 0.0;
}

void DamageState::OnSideHit(const int InHealthLost, const int InMaxHealth)
{
	++Stacks;
	SlowTimer.Start(Now(), Settings.SlowTime);
	SideLoss += std::max(0, InHealthLost);
	if ( InMaxHealth > 0 && SideLoss > Settings.StopShare * InMaxHealth )
	{
		StopTimer.Start(Now(), Settings.StopTime);
		SideLoss = 0;
	}
	LastScale = GetSpeedScale();
}

bool DamageState::Update()
{
	if ( SlowTimer.IsReady(Now()) )
	{
		Stacks = 0;
	}
	const float Scale = GetSpeedScale();
	const bool bChanged = Scale != LastScale;
	LastScale = Scale;
	return bChanged;
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
	return !StopTimer.IsReady(Now());
}

int DamageState::GetStacks() const
{
	return Stacks;
}

} // namespace tanks::model
