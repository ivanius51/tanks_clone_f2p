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
	// How often a move heads for the player's base instead of the player.
	float GetBaseFocusChance() const;
	void SetBaseFocusChance(float InChance);
	// A player who stays within CampRadius for CampTime is hunted down.
	float GetCampRadius() const;
	float GetCampTime() const;
	void SetCamp(float InRadius, float InTime);
	// A move towards the player is sometimes faked: a turn aside for FakeTurnTime, then back.
	float GetFakeTurnChance() const;
	float GetFakeTurnTime() const;
	void SetFakeTurn(float InChance, float InTime);
	// Health share at or below which it backs away from the player; 0 = never.
	float GetRetreatHealth() const;
	void SetRetreatHealth(float InShare);
	// Shots per second at a breakable thing ahead while driving, and how far ahead it is looked for.
	float GetClearWayRate() const;
	float GetClearWayTiles() const;
	void SetClearWay(float InRate, float InTiles);
	// How many open cells a breakable one is worth when finding a way: higher drives around walls more.
	int GetPathBreakCost() const;
	void SetPathBreakCost(int InCost);
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
	float BaseFocusChance = 0.0f;
	float CampRadius = 0.0f;
	float CampTime = 0.0f;
	float FakeTurnChance = 0.0f;
	float FakeTurnTime = 0.0f;
	float RetreatHealth = 0.0f;
	float DodgeChance = 0.0f;
	float ClearWayRate = 0.0f;
	float ClearWayTiles = 0.0f;
	int PathBreakCost = 1;
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
