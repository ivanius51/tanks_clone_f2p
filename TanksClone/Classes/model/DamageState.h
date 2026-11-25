#pragma once

#include "model/ConfigNode.h"
#include "model/Cooldown.h"
#include "model/SimulationClock.h"

namespace game::model
{
/** Numbers of side hits, from configs/balance/side_hits.json. */
struct SideHitSettings
{
	float SlowPerHit = 0.12f;
	float MaxSlow = 0.5f;
	float SlowTime = 3.0f;
	float StopShare = 0.5f;
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
} // namespace game::model
