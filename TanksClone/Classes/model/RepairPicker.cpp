#include "model/RepairPicker.h"

#include <algorithm>

namespace tanks::model
{

std::vector<int> RepairPicker::Pick(const std::vector<RepairCandidate>& InCandidates, const float InReach, const int InMaxTargets)
{
	std::vector<int> Picked;
	for ( int Index = 0; Index < static_cast<int>(InCandidates.size()); ++Index )
	{
		const RepairCandidate& Candidate = InCandidates[Index];
		if ( Candidate.MaxHealth > 0 && Candidate.Health > 0 && Candidate.Health < Candidate.MaxHealth && Candidate.Distance <= InReach )
		{
			Picked.push_back(Index);
		}
	}
	const auto Share = [&InCandidates](const int InIndex)
	{
		return static_cast<float>(InCandidates[InIndex].Health) / InCandidates[InIndex].MaxHealth;
	};
	std::stable_sort(Picked.begin(), Picked.end(), [&Share](const int InLeft, const int InRight) { return Share(InLeft) < Share(InRight); });
	if ( InMaxTargets >= 0 && static_cast<int>(Picked.size()) > InMaxTargets )
	{
		Picked.resize(InMaxTargets);
	}
	return Picked;
}

} // namespace tanks::model
