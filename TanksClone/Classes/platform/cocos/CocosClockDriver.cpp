#include "platform/cocos/CocosClockDriver.h"

#include "cocos2d.h"

#include "core/GameContext.h"
#include "model/Scheduler.h"
#include "model/SimulationClock.h"

namespace tanks::view::cocos
{

CocosClockDriver::CocosClockDriver()
{
	cocos2d::Director::getInstance()->getScheduler()->scheduleUpdate(this, cocos2d::Scheduler::PRIORITY_SYSTEM, false);
}

CocosClockDriver::~CocosClockDriver()
{
	cocos2d::Director::getInstance()->getScheduler()->unscheduleUpdate(this);
}

void CocosClockDriver::update(const float InDelta)
{
	model::SimulationClock& Clock = GameContext::Get().GetManager<model::SimulationClock>();
	Clock.Advance(InDelta);
	GameContext::Get().GetManager<model::Scheduler>().RunDue(Clock.GetNow());
}

} // namespace tanks::view::cocos
