#pragma once

#include "model/MathTypes.h"

namespace tanks::model
{

/**
 * Notices a player who sits in one spot: the time spent within Radius of the place where the player
 * stopped. Moving out of that circle starts the count again from the new place.
 */
class CampWatch
{
public:
	void Configure(float InRadius, float InTime);
	void Update(const Vec2f& InPosition, float InDelta);
	bool IsCamping() const;
	float GetStillTime() const;

private:
	float Radius = 0.0f;
	float Time = 0.0f;
	float StillTime = 0.0f;
	Vec2f Anchor;
	bool bHasAnchor = false;
};

} // namespace tanks::model
