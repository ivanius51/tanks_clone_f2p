#pragma once
#include "templates/TContainers.h"


namespace game::model
{
/**
 * Tells when a boss falls below the health shares that start its next phases.
 * Each phase starts once, in order, even if one hit skips past several thresholds.
 */
class PhaseTracker
{
public:
	void Configure(const TArray<float>& InThresholds);
	TArray<int> Update(float InHealthShare);
	int GetCurrentPhase() const;

private:
	TArray<float> Thresholds;
	int NextPhase = 0;
};
} // namespace game::model
