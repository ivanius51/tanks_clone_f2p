#pragma once

#include "model/Animation.h"

namespace tanks::model
{

/**
 * The animated value of a bar: the shown share eases to a new value (a full bar takes FullTime seconds, shaped by an
 * Easing); when the value drops, a trail keeps the lost share and drains after TrailDelay seconds. Engine-free.
 */
class BarMeter
{
public:
	BarMeter() = default;
	BarMeter(float InFullTime, float InTrailDelay, Easing InEase);

	// The first value, or a zero full time, jumps; later values ease.
	void SetTarget(float InFraction);
	void Advance(float InDelta);

	float GetTarget() const;
	float GetShown() const;
	float GetTrail() const;
	bool IsSettled() const;

private:
	float FullTime = 0.0f;
	float TrailDelay = 0.0f;
	Easing Ease = Easing::Linear;
	bool bHasTarget = false;
	float Target = 0.0f;
	float From = 0.0f;
	float Shown = 0.0f;
	float Trail = 0.0f;
	float Elapsed = 0.0f;
	float Duration = 0.0f;
	float TrailHold = 0.0f;
};

} // namespace tanks::model
