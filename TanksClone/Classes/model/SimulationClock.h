#pragma once

namespace tanks::model
{
/**
 * Game time in seconds: advanced once per simulation step by whoever runs the game, never by the wall clock, so
 * pause, time scale and replays move it correctly. Cooldowns store timestamps of this clock instead of counting down.
 */
class SimulationClock
{
public:
	void Advance(const float InDelta)
	{
		Seconds += InDelta;
	}

	double GetNow() const
	{
		return Seconds;
	}

private:
	double Seconds = 0.0;
};
} // namespace tanks::model
