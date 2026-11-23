#pragma once

#include <algorithm>

namespace tanks::model
{

/**
 * A one-shot wait: Start sets how long, Tick runs it down to zero, IsReady says it is over.
 * Readiness runs 0 (just started) to 1 (ready) for the bars that show it.
 */
class Cooldown
{
public:
	void Start(const float InDuration)
	{
		Length = std::max(0.0f, InDuration);
		Remaining = Length;
	}

	void Tick(const float InDelta)
	{
		Remaining = std::max(0.0f, Remaining - InDelta);
	}

	bool IsReady() const
	{
		return Remaining <= 0.0f;
	}

	float GetRemaining() const
	{
		return Remaining;
	}

	float GetReadiness() const
	{
		return Length > 0.0f ? 1.0f - std::min(1.0f, Remaining / Length) : 1.0f;
	}

private:
	float Length = 0.0f;
	float Remaining = 0.0f;
};

} // namespace tanks::model
