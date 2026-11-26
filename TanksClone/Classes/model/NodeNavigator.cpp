#include "model/NodeNavigator.h"
#include "templates/TContainers.h"

#include <cmath>

namespace game::model
{

namespace
{
	// Sideways distance counts this much more than distance along the pressed direction.
	constexpr float SideWeight = 2.0f;
	constexpr float MinAlong = 1.0f;
	// A point further sideways than this many times its distance ahead is not "that way".
	constexpr float MaxSideRatio = 2.0f;
}

int NodeNavigator::FindNext(const TArray<Vec2f>& InPoints, const TArray<uint8_t>& InSelectable, const int InFrom, const Vec2f& InDirection)
{
	const int Count = static_cast<int>(InPoints.Size());
	if ( InFrom < 0 || InFrom >= Count )
	{
		return InFrom;
	}
	const Vec2f Direction = InDirection.Normalized();
	int Best = InFrom;
	float BestScore = 0.0f;
	for ( int Index = 0; Index < Count; ++Index )
	{
		if ( Index == InFrom || (!InSelectable.IsEmpty() && InSelectable[Index] == 0) )
		{
			continue;
		}
		const Vec2f Offset = InPoints[Index] - InPoints[InFrom];
		const float Along = Offset.Dot(Direction);
		if ( Along < MinAlong )
		{
			continue;
		}
		const float Side = std::fabs(Offset.X * Direction.Y - Offset.Y * Direction.X);
		if ( Side > Along * MaxSideRatio )
		{
			continue;
		}
		const float Score = Along + Side * SideWeight;
		if ( Best == InFrom || Score < BestScore )
		{
			Best = Index;
			BestScore = Score;
		}
	}
	return Best;
}

} // namespace game::model
