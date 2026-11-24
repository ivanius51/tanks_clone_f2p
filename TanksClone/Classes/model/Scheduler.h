#pragma once

#include <cstddef>
#include <vector>

#include "templates/TFunction.h"

namespace tanks::model
{
/**
 * Runs callbacks at a game time: At a given time, or After a delay counted from the time of the last RunDue.
 * Whoever steps the simulation calls RunDue(Now) once per step with the SimulationClock time. Callbacks due at the
 * same time run in the order they were added; a callback may schedule more. Engine free, so it is unit-tested.
 */
class Scheduler
{
public:
	using Callback = TFunction<void()>;
	using Handle = unsigned;

	Handle At(double InTime, Callback InCallback);
	Handle After(float InDelay, Callback InCallback);
	bool Cancel(Handle InHandle);
	void Clear();

	void RunDue(double InNow);
	std::size_t GetPendingCount() const;

private:
	struct Entry
	{
		Handle Id = 0;
		double Time = 0.0;
		Callback Action;
	};

	std::vector<Entry> Pending;
	Handle NextId = 1;
	double LastNow = 0.0;
};
} // namespace tanks::model
