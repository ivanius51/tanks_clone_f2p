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
	// Health shares (0..1) that start each phase, in phase order.
	void Configure(const std::vector<float>& InThresholds);
	// Phases started by this health share, in order; empty if none.
	std::vector<int> Update(float InHealthShare);
	// -1 before the first phase.
	int GetCurrentPhase() const;

private:
	std::vector<float> Thresholds;
	int NextPhase = 0;
};

} // namespace tanks::model
