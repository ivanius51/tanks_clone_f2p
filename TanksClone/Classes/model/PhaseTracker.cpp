#include "model/PhaseTracker.h"

namespace tanks::model
{

void PhaseTracker::Configure(const std::vector<float>& InThresholds)
{
	Thresholds = InThresholds;
	NextPhase = 0;
}

std::vector<int> PhaseTracker::Update(const float InHealthShare)
{
	std::vector<int> Started;
	while ( NextPhase < static_cast<int>(Thresholds.size()) && InHealthShare <= Thresholds[NextPhase] )
	{
		Started.push_back(NextPhase++);
	}
	return Started;
}

int PhaseTracker::GetCurrentPhase() const
{
	return NextPhase - 1;
}

} // namespace tanks::model
