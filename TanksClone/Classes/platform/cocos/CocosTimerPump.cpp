#include "platform/cocos/CocosTimerPump.h"

#include "cocos2d.h"

#include "core/GameContext.h"
#include "model/TimerManager.h"

namespace game::view::cocos
{

CocosTimerPump::CocosTimerPump()
{
	cocos2d::Director::getInstance()->getScheduler()->scheduleUpdate(this, cocos2d::Scheduler::PRIORITY_SYSTEM, false);
}

CocosTimerPump::~CocosTimerPump()
{
	cocos2d::Director::getInstance()->getScheduler()->unscheduleUpdate(this);
}

void CocosTimerPump::update(float /*InDelta*/)
{
	GameContext::Get<model::TimerManager>().Update();
}

} // namespace game::view::cocos
