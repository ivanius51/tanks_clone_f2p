#include "model/BarMeter.h"

#include <algorithm>
#include <cmath>

namespace tanks::model
{

BarMeter::BarMeter(const float InFullTime, const float InTrailDelay, const Easing InEase)
	: FullTime(std::max(InFullTime, 0.0f))
	, TrailDelay(std::max(InTrailDelay, 0.0f))
	, Ease(InEase)
{
}

void BarMeter::SetTarget(const float InFraction)
{
	const float Fraction = std::clamp(InFraction, 0.0f, 1.0f);
	if ( bHasTarget && Fraction == Target )
	{
		return;
	}
	const bool bJump = !bHasTarget || FullTime <= 0.0f;
	bHasTarget = true;
	Target = Fraction;
	if ( bJump )
	{
		Shown = Trail = From = Fraction;
		Elapsed = Duration = TrailHold = 0.0f;
		return;
	}
	if ( Fraction < Shown )
	{
		TrailHold = TrailDelay;
	}
	else
	{
		Trail = std::max(Trail, Fraction);
	}
	From = Shown;
	Elapsed = 0.0f;
	Duration = FullTime * std::abs(Fraction - From);
}

void BarMeter::Advance(const float InDelta)
{
	if ( IsSettled() )
	{
		return;
	}
	if ( Shown != Target )
	{
		Elapsed += InDelta;
		const float Share = Duration > 0.0f ? std::min(Elapsed / Duration, 1.0f) : 1.0f;
		Shown = Share >= 1.0f ? Target : From + (Target - From) * ApplyEasing(Ease, Share);
	}
	if ( TrailHold > 0.0f )
	{
		TrailHold = std::max(TrailHold - InDelta, 0.0f);
	}
	else if ( FullTime > 0.0f )
	{
		const float Step = InDelta / FullTime;
		Trail = Trail < Shown ? std::min(Trail + Step, Shown) : std::max(Trail - Step, Shown);
	}
	Trail = std::max(Trail, Shown);
}

float BarMeter::GetTarget() const
{
	return Target;
}

float BarMeter::GetShown() const
{
	return Shown;
}

float BarMeter::GetTrail() const
{
	return Trail;
}

bool BarMeter::IsSettled() const
{
	return Shown == Target && Trail == Shown && TrailHold <= 0.0f;
}

} // namespace tanks::model
