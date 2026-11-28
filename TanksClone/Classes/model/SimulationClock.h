#pragma once

#include <chrono>
#include <cstdint>

#include "templates/TTemplateHelper.h"

namespace game::model
{
/**
 * Unified simulation clock owned by GameContext:
 * - RuntimeSeconds : Unpaused real time since start (menus, UI animations, paused timers, loading screens).
 * - GameplaySeconds: Scaled, pause-aware simulation time (cooldowns, buffs, physics).
 * - GameplayTick   : Deterministic integer step counter (replays, input history, GameEvents).
 */
class SimulationClock : public FNonCopyableNonMovable
{
public:
	/** The tickable-manager entry: advances gameplay time by a frame time in milliseconds. */
	void Tick(const float InDeltaMs, std::chrono::microseconds /*InBudget*/) noexcept
	{
		Advance(InDeltaMs / 1000.0f);
	}

	void Advance(const float InScaledDelta) noexcept
	{
		if (InScaledDelta <= 0.0f)
		{
			return;
		}
		GameplaySeconds += static_cast<double>(InScaledDelta);
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
	// Read from the monotonic system clock, so it is right on any thread and needs no per-frame update.
	double GetRuntimeSeconds() const noexcept
	{
		return std::chrono::duration<double>(std::chrono::steady_clock::now() - Start).count();
	}

private:
	double GameplaySeconds = 0.0;
	std::chrono::steady_clock::time_point Start = std::chrono::steady_clock::now();
	std::uint64_t GameplayTick = 0;
};
} // namespace game::model