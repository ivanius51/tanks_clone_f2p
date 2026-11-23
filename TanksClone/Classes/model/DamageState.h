#pragma once

#include "model/ConfigNode.h"
#include "model/Cooldown.h"
#include "model/SimulationClock.h"

namespace tanks::model
{

/** Numbers of side hits, from configs/balance/side_hits.json. */
struct SideHitSettings
{
	float SlowPerHit = 0.12f;   // share of speed lost per stacked side hit
	float MaxSlow = 0.5f;
	float SlowTime = 3.0f;      // seconds without a side hit before the stacks clear
	float StopShare = 0.5f;     // share of max health lost to side hits that stops the tank
	float StopTime = 2.0f;

	static SideHitSettings FromConfig(const IConfigNode& InNode);
};

/**
 * Hidden effect of shots into a tank's sides: each one stacks a slow that clears after a quiet spell,
 * and losing more than a share of max health to side hits stops the tank for a while.
 */
class DamageState
{
public:
	DamageState() = default;
	DamageState(const SideHitSettings& InSettings, const SimulationClock& InClock);

	void OnSideHit(int InHealthLost, int InMaxHealth);
	// Clears expired effects; returns true when the speed scale changed since the last call.
	bool Update();
	float GetSpeedScale() const;
	bool IsImmobilized() const;
	int GetStacks() const;

private:
	double Now() const;

	SideHitSettings Settings;
	const SimulationClock* Clock = nullptr;
	float LastScale = 1.0f;
	int Stacks = 0;
	int SideLoss = 0;
	Cooldown SlowTimer;
	Cooldown StopTimer;
};

} // namespace tanks::model
