#include "game/ai/AiSettings.h"

#include <algorithm>

namespace tanks
{

float TankAiSettings::GetThinkIntervalMin() const
{
	return ThinkIntervalMin;
}

float TankAiSettings::GetThinkIntervalMax() const
{
	return ThinkIntervalMax;
}

void TankAiSettings::SetThinkInterval(const float InMin, const float InMax)
{
	ThinkIntervalMin = std::min(InMin, InMax);
	ThinkIntervalMax = std::max(InMin, InMax);
}

float TankAiSettings::GetFocusOnPlayerChance() const
{
	return FocusOnPlayerChance;
}

void TankAiSettings::SetFocusOnPlayerChance(const float InChance)
{
	FocusOnPlayerChance = std::clamp(InChance, 0.0f, 1.0f);
}

float TankAiSettings::GetRandomFireRate() const
{
	return RandomFireRate;
}

void TankAiSettings::SetRandomFireRate(const float InRate)
{
	RandomFireRate = std::max(InRate, 0.0f);
}

float TankAiSettings::GetSecondaryAbilityRate() const
{
	return SecondaryAbilityRate;
}

void TankAiSettings::SetSecondaryAbilityRate(const float InRate)
{
	SecondaryAbilityRate = std::max(InRate, 0.0f);
}

float TankAiSettings::GetAimTolerance() const
{
	return AimTolerance;
}

void TankAiSettings::SetAimTolerance(const float InTolerance)
{
	AimTolerance = std::max(InTolerance, 0.0f);
}

float TankAiSettings::GetCellCenterTolerance() const
{
	return CellCenterTolerance;
}

void TankAiSettings::SetCellCenterTolerance(const float InTolerance)
{
	CellCenterTolerance = std::max(InTolerance, 0.0f);
}

float TankAiSettings::GetMaxTurnWait() const
{
	return MaxTurnWait;
}

void TankAiSettings::SetMaxTurnWait(const float InSeconds)
{
	MaxTurnWait = std::max(InSeconds, 0.0f);
}

float PlayerBotSettings::GetThinkInterval() const
{
	return ThinkInterval;
}

void PlayerBotSettings::SetThinkInterval(const float InSeconds)
{
	ThinkInterval = std::max(InSeconds, 0.0f);
}

float PlayerBotSettings::GetAimTolerance() const
{
	return AimTolerance;
}

void PlayerBotSettings::SetAimTolerance(const float InTolerance)
{
	AimTolerance = std::max(InTolerance, 0.0f);
}

float PlayerBotSettings::GetAbilityRange() const
{
	return AbilityRange;
}

void PlayerBotSettings::SetAbilityRange(const float InRange)
{
	AbilityRange = std::max(InRange, 0.0f);
}

float PlayerBotSettings::GetHoldDistance() const
{
	return HoldDistance;
}

void PlayerBotSettings::SetHoldDistance(const float InDistance)
{
	HoldDistance = std::max(InDistance, 0.0f);
}

float PlayerBotSettings::GetStuckTime() const
{
	return StuckTime;
}

void PlayerBotSettings::SetStuckTime(const float InSeconds)
{
	StuckTime = std::max(InSeconds, 0.0f);
}

float PlayerBotSettings::GetHomeHeight() const
{
	return HomeHeight;
}

void PlayerBotSettings::SetHomeHeight(const float InHeight)
{
	HomeHeight = std::max(InHeight, 0.0f);
}

float PlayerBotSettings::GetFacingAlignment() const
{
	return FacingAlignment;
}

void PlayerBotSettings::SetFacingAlignment(const float InAlignment)
{
	FacingAlignment = std::clamp(InAlignment, -1.0f, 1.0f);
}

} // namespace tanks
