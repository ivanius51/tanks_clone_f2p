#include "model/PhaseTracker.h"
#include "templates/TContainers.h"

namespace game::model
{

void PhaseTracker::Configure(const TArray<float>& InThresholds)
{
	Thresholds = InThresholds;
	NextPhase = 0;
}

TArray<int> PhaseTracker::Update(const float InHealthShare)
{
	TArray<int> Started;
	while ( NextPhase < static_cast<int>(Thresholds.Size()) && InHealthShare <= Thresholds[NextPhase] )
	{
		Started.PushBack(NextPhase++);
	}
	return Started;
}

int PhaseTracker::GetCurrentPhase() const
{
	return NextPhase - 1;
}

} // namespace game::model
