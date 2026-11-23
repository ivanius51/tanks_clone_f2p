#include "model/DropTimer.h"

namespace tanks::model
{

void DropTimer::Configure(const float InInterval)
{
	Timer.Configure(InInterval);
}

bool DropTimer::Update(const float InDelta, const bool bInMoving)
{
	return bInMoving && Timer.Tick(InDelta);
}

float DropTimer::GetReadiness() const
{
	return Timer.GetReadiness();
}

float DropTimer::GetRemaining() const
{
	return Timer.GetRemaining();
}

} // namespace tanks::model
