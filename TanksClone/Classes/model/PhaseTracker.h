#pragma once

#include <vector>

namespace tanks::model
{
/**
 * Tells when a boss falls below the health shares that start its next phases.
 * Each phase starts once, in order, even if one hit skips past several thresholds.
 */
class PhaseTracker
{
public:
	void Configure(const std::vector<float>& InThresholds);
	std::vector<int> Update(float InHealthShare);
	int GetCurrentPhase() const;

private:
	std::vector<float> Thresholds;
	int NextPhase = 0;
};
} // namespace tanks::model
