#include "platform/cocos/CocosClockDriver.h"

#include "cocos2d.h"

#include <algorithm>

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
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
	if ( !bBudgetRead && GameContext::Get<ConfigReader>().HasConfig(ResourcesConfig) )
	{
		const float BudgetMilliseconds = GameContext::Get<ConfigReader>().GetConfig(ResourcesConfig).GetValueOr("tick_budget_ms", 12.0f);
		TickBudget = std::chrono::microseconds(static_cast<long long>(std::max(0.5f, BudgetMilliseconds) * 1000.0f));
		bBudgetRead = true;
	}
	GameContext::Get().Tick(InDelta * 1000.0f, TickBudget);
}

} // namespace game::view::cocos
