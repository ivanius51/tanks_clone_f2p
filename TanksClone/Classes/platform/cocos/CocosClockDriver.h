#pragma once

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
};

} // namespace game::view::cocos
