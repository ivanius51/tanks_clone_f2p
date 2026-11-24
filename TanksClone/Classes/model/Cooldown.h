#pragma once

#include <algorithm>

namespace tanks::model
{
/**
 * A one-shot wait kept as timestamps of a SimulationClock: Start stores when it began and ends, nothing is ticked.
 * Readiness runs 0 (just started) to 1 (ready) for the bars that show it.
 */
class Cooldown
{
public:
	void Start(const double InNow, const float InDuration)
	{
		Length = std::max(0.0f, InDuration);
		EndTime = InNow + Length;
	}

	bool IsReady(const double InNow) const
	{
		return InNow >= EndTime;
	}

	float GetRemaining(const double InNow) const
	{
		return static_cast<float>(std::max(0.0, EndTime - InNow));
	}

	float GetReadiness(const double InNow) const
	{
		return Length > 0.0f ? 1.0f - std::min(1.0f, GetRemaining(InNow) / Length) : 1.0f;
	}

private:
	float Length = 0.0f;
	double EndTime = 0.0;
};
} // namespace tanks::model
