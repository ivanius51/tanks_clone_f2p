#include "model/CampWatch.h"

#include <algorithm>

namespace game::model
{

void CampWatch::Configure(const float InRadius, const float InTime)
{
	Radius = std::max(0.0f, InRadius);
	Time = std::max(0.0f, InTime);
}

void CampWatch::Update(const Vec2f& InPosition, const float InDelta)
{
	const Vec2f Offset = InPosition - Anchor;
	if ( !bHasAnchor || Offset.X * Offset.X + Offset.Y * Offset.Y > Radius * Radius )
	{
		Anchor = InPosition;
		bHasAnchor = true;
		StillTime = 0.0f;
		return;
	}
	StillTime += InDelta;
}

bool CampWatch::IsCamping() const
{
	return Time > 0.0f && StillTime >= Time;
}

float CampWatch::GetStillTime() const
{
	return StillTime;
}

} // namespace game::model
