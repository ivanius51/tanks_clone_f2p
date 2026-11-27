#pragma once

#include <chrono>

namespace game::view::cocos
{

/**
 * Calls GameContext::Tick (the model's own update) once per engine scheduler step, before everything else updates.
 * Pause and time scale of the engine therefore move game time the same way, in the game and in tests.
 */
class CocosClockDriver
{
public:
	CocosClockDriver();
	~CocosClockDriver();
	CocosClockDriver(const CocosClockDriver&) = delete;
	CocosClockDriver& operator=(const CocosClockDriver&) = delete;

	void update(float InDelta);

private:
	/** Wall-time limit of one GameContext::Tick, from tick_budget_ms of resources.json (read once the config is loaded). */
	std::chrono::microseconds TickBudget = std::chrono::microseconds::max();
	bool bBudgetRead = false;
};

} // namespace game::view::cocos
