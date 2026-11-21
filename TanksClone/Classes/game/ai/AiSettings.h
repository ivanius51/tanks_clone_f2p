#pragma once

#include "model/AiFeatureSet.h"

namespace tanks
{

/** Tuning of an enemy tank's AI (one profile per AiType). Engine- and JSON-free; AiSettingsLoader fills it. */
class TankAiSettings
{
public:
	float GetThinkIntervalMin() const;
	float GetThinkIntervalMax() const;
	void SetThinkInterval(float InMin, float InMax);
	float GetFocusOnPlayerChance() const;
	void SetFocusOnPlayerChance(float InChance);
	float GetRandomFireRate() const;
	void SetRandomFireRate(float InRate);
	float GetSecondaryAbilityRate() const;
	void SetSecondaryAbilityRate(float InRate);
	float GetAimTolerance() const;
	void SetAimTolerance(float InTolerance);
	float GetCellCenterTolerance() const;
	void SetCellCenterTolerance(float InTolerance);
	float GetMaxTurnWait() const;
	void SetMaxTurnWait(float InSeconds);
	// Which behaviours the profile turns on ("ai_features").
	const model::AiSwitches& GetFeatures() const;
	void SetFeatures(const model::AiSwitches& InFeatures);
	// A pickup closer than GuardRange draws it in; it then circles within GuardRadius of it.
	float GetGuardRange() const;
	float GetGuardRadius() const;
	void SetGuard(float InRange, float InRadius);
	// Health share at or below which it backs away from the player; 0 = never.
	float GetRetreatHealth() const;
	void SetRetreatHealth(float InShare);
	float GetDodgeChance() const;
	void SetDodgeChance(float InChance);
	float GetDodgeLookahead() const;
	void SetDodgeLookahead(float InSeconds);
	// Chance to come at the player from the side instead of head on.
	float GetFlankChance() const;
	void SetFlankChance(float InChance);

private:
	float ThinkIntervalMin = 0.0f;
	float ThinkIntervalMax = 0.0f;
	float FocusOnPlayerChance = 0.0f;
	float RandomFireRate = 0.0f;
	float SecondaryAbilityRate = 0.0f;
	float AimTolerance = 0.0f;
	float CellCenterTolerance = 0.0f;
	float MaxTurnWait = 0.0f;
	model::AiSwitches Features;
	float GuardRange = 0.0f;
	float GuardRadius = 0.0f;
	float RetreatHealth = 0.0f;
	float DodgeChance = 0.0f;
	float DodgeLookahead = 0.0f;
	float FlankChance = 0.0f;
};

/** Tuning of the test bot that plays for the player. Engine- and JSON-free; AiSettingsLoader fills it. */
class PlayerBotSettings
{
public:
	float GetThinkInterval() const;
	void SetThinkInterval(float InSeconds);
	float GetAimTolerance() const;
	void SetAimTolerance(float InTolerance);
	float GetAbilityRange() const;
	void SetAbilityRange(float InRange);
	float GetHoldDistance() const;
	void SetHoldDistance(float InDistance);
	float GetStuckTime() const;
	void SetStuckTime(float InSeconds);
	float GetHomeHeight() const;
	void SetHomeHeight(float InHeight);
	float GetFacingAlignment() const;
	void SetFacingAlignment(float InAlignment);

private:
	float ThinkInterval = 0.0f;
	float AimTolerance = 0.0f;
	float AbilityRange = 0.0f;
	float HoldDistance = 0.0f;
	float StuckTime = 0.0f;
	float HomeHeight = 0.0f;
	float FacingAlignment = 0.0f;
};

} // namespace tanks
