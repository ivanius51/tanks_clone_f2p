#pragma once

#include <algorithm>

namespace tanks::model
{

/**
 * A repeating timer: counts down an interval and tells when it ran out. The shared base of the timers that
 * show a readiness bar (0 = just restarted, 1 = due).
 */
class Countdown
{
public:
	void Configure(const float InInterval)
	{
		Interval = std::max(0.0f, InInterval);
		Remaining = Interval;
	}

	// True when the interval ran out this step; it then starts the next one.
	bool Tick(const float InDelta)
	{
		Remaining -= InDelta;
		if ( Remaining > 0.0f )
		{
			return false;
		}
		Remaining = Interval;
		return true;
	}

	float GetInterval() const
	{
		return Interval;
	}

	float GetRemaining() const
	{
		return std::max(0.0f, Remaining);
	}

	float GetReadiness() const
	{
		return Interval > 0.0f ? 1.0f - std::clamp(Remaining / Interval, 0.0f, 1.0f) : 1.0f;
	}

private:
	float Interval = 1.0f;
	float Remaining = 1.0f;
};

} // namespace tanks::model
