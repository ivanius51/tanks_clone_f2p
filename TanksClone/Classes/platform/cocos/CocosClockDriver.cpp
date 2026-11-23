#include "platform/cocos/CocosClockDriver.h"

#include "cocos2d.h"

#include "core/GameContext.h"
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
	GameContext::Get().GetManager<model::SimulationClock>().Advance(InDelta);
}

} // namespace tanks::view::cocos
