#include "model/RepairPicker.h"
#include "templates/TContainers.h"

#include <algorithm>

namespace game::model
{

TArray<int> RepairPicker::Pick(const TArray<RepairCandidate>& InCandidates, const float InReach, const int InMaxTargets)
{
	TArray<int> Picked;
	for ( int Index = 0; Index < static_cast<int>(InCandidates.Size()); ++Index )
	{
		const RepairCandidate& Candidate = InCandidates[Index];
		if ( Candidate.MaxHealth > 0 && Candidate.Health > 0 && Candidate.Health < Candidate.MaxHealth && Candidate.Distance <= InReach )
		{
			Picked.PushBack(Index);
		}
	}
	const auto Share = [&InCandidates](const int InIndex)
	{
		return static_cast<float>(InCandidates[InIndex].Health) / InCandidates[InIndex].MaxHealth;
	};
	std::stable_sort(Picked.begin(), Picked.end(), [&Share](const int InLeft, const int InRight) { return Share(InLeft) < Share(InRight); });
	if ( InMaxTargets >= 0 && static_cast<int>(Picked.Size()) > InMaxTargets )
	{
		Picked.Resize(InMaxTargets);
	}
	return Picked;
}

} // namespace game::model
