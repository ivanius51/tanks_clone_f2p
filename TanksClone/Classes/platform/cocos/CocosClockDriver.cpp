#include "platform/cocos/CocosClockDriver.h"

#include "cocos2d.h"

#include "core/GameContext.h"
#include "model/Scheduler.h"
#include "model/SimulationClock.h"

namespace game::view::cocos
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
	GameContext::Get().Tick(InDelta);
}

} // namespace game::view::cocos
