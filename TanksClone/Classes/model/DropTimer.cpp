#include "model/DropTimer.h"

#include <algorithm>

namespace tanks::model
{

void DropTimer::Configure(const float InInterval)
{
	Interval = std::max(0.0f, InInterval);
	Remaining = Interval;
}

bool DropTimer::Update(const float InDelta, const bool bInMoving)
{
	if ( !bInMoving )
	{
		return false;
	}
	Remaining -= InDelta;
	if ( Remaining > 0.0f )
	{
		return false;
	}
	Remaining = Interval;
	return true;
}

float DropTimer::GetReadiness() const
{
	return Interval > 0.0f ? 1.0f - std::clamp(Remaining / Interval, 0.0f, 1.0f) : 1.0f;
}

float DropTimer::GetRemaining() const
{
	return std::max(0.0f, Remaining);
}

} // namespace tanks::model
