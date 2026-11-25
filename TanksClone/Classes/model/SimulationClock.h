#pragma once

#include <cstdint>

#include "templates/TTemplateHelper.h"

namespace game::model
{
/**
 * Unified simulation clock owned by GameContext:
 * - RuntimeSeconds : Unpaused engine uptime (menus, UI animations, paused timers).
 * - GameplaySeconds: Scaled, pause-aware simulation time (cooldowns, buffs, physics).
 * - GameplayTick   : Deterministic integer step counter (replays, input history, GameEvents).
 */
class SimulationClock : public FNonCopyableNonMovable
{
public:
	void AdvanceRuntime(const float InUnscaledDelta) noexcept
	{
		if (InUnscaledDelta > 0.0f)
		{
			RuntimeSeconds += static_cast<double>(InUnscaledDelta);
		}
	}

	void Advance(const float InScaledDelta) noexcept
	{
		if (InScaledDelta > 0.0f)
		{
			GameplaySeconds += static_cast<double>(InScaledDelta);
		}
		++GameplayTick;
	}

	void ResetGameplay() noexcept
	{
		GameplaySeconds = 0.0;
		GameplayTick = 0;
	}

	// Gameplay time (pauses when game is paused, affected by time scale)
	double GetNow() const noexcept { return GameplaySeconds; }
	double GetGameplaySeconds() const noexcept { return GameplaySeconds; }
	std::uint64_t GetGameplayTick() const noexcept { return GameplayTick; }

	// Total runtime including menus and pauses
	double GetRuntimeSeconds() const noexcept { return RuntimeSeconds; }

private:
	double GameplaySeconds = 0.0;
	double RuntimeSeconds = 0.0;
	std::uint64_t GameplayTick = 0;
};
} // namespace game::model