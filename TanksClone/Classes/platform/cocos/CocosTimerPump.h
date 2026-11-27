#pragma once

namespace game::view::cocos
{

/** Calls model::TimerManager::Update() once per engine step, on the main thread, before everything else updates. */
class CocosTimerPump
{
public:
	CocosTimerPump();
	~CocosTimerPump();
	CocosTimerPump(const CocosTimerPump&) = delete;
	CocosTimerPump& operator=(const CocosTimerPump&) = delete;

	void update(float InDelta);
};

} // namespace game::view::cocos
