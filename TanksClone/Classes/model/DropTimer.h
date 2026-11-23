#pragma once

#include "model/Countdown.h"

namespace tanks::model
{

/**
 * Paces something left behind on the move (mines): one drop every interval of moving time.
 * Standing still pauses the count, so a parked vehicle does not pile drops on one spot.
 */
class DropTimer
{
public:
	void Configure(float InInterval);
	// True when a drop is due this step; it then starts the next interval.
	bool Update(float InDelta, bool bInMoving);
	// 0 = just dropped, 1 = due.
	float GetReadiness() const;
	float GetRemaining() const;

private:
	Countdown Timer;
};

} // namespace tanks::model
