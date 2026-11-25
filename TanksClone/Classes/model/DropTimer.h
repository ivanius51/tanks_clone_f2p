#pragma once

#include "model/Countdown.h"

namespace game::model
{
/**
 * Paces something left behind on the move (mines): one drop every interval of moving time.
 * Standing still pauses the count, so a parked vehicle does not pile drops on one spot.
 */
class DropTimer
{
public:
	void Configure(float InInterval);
	bool Update(float InDelta, bool bInMoving);
	float GetReadiness() const;
	float GetRemaining() const;

private:
	Countdown Timer;
};
} // namespace game::model
