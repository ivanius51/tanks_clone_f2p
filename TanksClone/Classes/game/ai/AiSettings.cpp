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

const model::AiSwitches& TankAiSettings::GetFeatures() const
{
	return Features;
}

void TankAiSettings::SetFeatures(const model::AiSwitches& InFeatures)
{
	Features = InFeatures;
}

float TankAiSettings::GetGuardRange() const
{
	return GuardRange;
}

float TankAiSettings::GetGuardRadius() const
{
	return GuardRadius;
}

void TankAiSettings::SetGuard(const float InRange, const float InRadius)
{
	GuardRange = std::max(0.0f, InRange);
	GuardRadius = std::max(0.0f, InRadius);
}

float TankAiSettings::GetBaseFocusChance() const
{
	return BaseFocusChance;
}

void TankAiSettings::SetBaseFocusChance(const float InChance)
{
	BaseFocusChance = std::clamp(InChance, 0.0f, 1.0f);
}

float TankAiSettings::GetCampRadius() const
{
	return CampRadius;
}

float TankAiSettings::GetCampTime() const
{
	return CampTime;
}

void TankAiSettings::SetCamp(const float InRadius, const float InTime)
{
	CampRadius = std::max(0.0f, InRadius);
	CampTime = std::max(0.0f, InTime);
}

float TankAiSettings::GetFakeTurnChance() const
{
	return FakeTurnChance;
}

float TankAiSettings::GetFakeTurnTime() const
{
	return FakeTurnTime;
}

void TankAiSettings::SetFakeTurn(const float InChance, const float InTime)
{
	FakeTurnChance = std::clamp(InChance, 0.0f, 1.0f);
	FakeTurnTime = std::max(0.0f, InTime);
}

float TankAiSettings::GetRetreatHealth() const
{
	return RetreatHealth;
}

void TankAiSettings::SetRetreatHealth(const float InShare)
{
	RetreatHealth = std::clamp(InShare, 0.0f, 1.0f);
}

float TankAiSettings::GetDodgeChance() const
{
	return DodgeChance;
}

void TankAiSettings::SetDodgeChance(const float InChance)
{
	DodgeChance = std::clamp(InChance, 0.0f, 1.0f);
}

float TankAiSettings::GetDodgeLookahead() const
{
	return DodgeLookahead;
}

void TankAiSettings::SetDodgeLookahead(const float InSeconds)
{
	DodgeLookahead = std::max(0.0f, InSeconds);
}

float TankAiSettings::GetFlankChance() const
{
	return FlankChance;
}

void TankAiSettings::SetFlankChance(const float InChance)
{
	FlankChance = std::clamp(InChance, 0.0f, 1.0f);
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
