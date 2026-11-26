#pragma once
#include "templates/TContainers.h"


namespace game::model
{
/** An ally a repairer could fix: its health and how far it is. */
struct RepairCandidate
{
	int Health = 0;
	int MaxHealth = 0;
	float Distance = 0.0f;
};

/**
 * Chooses whom a repairer fixes: damaged allies within reach, the most hurt (lowest health share) first,
 * at most a set number of them.
 */
class RepairPicker
{
public:
	static TArray<int> Pick(const TArray<RepairCandidate>& InCandidates, float InReach, int InMaxTargets);
};
} // namespace game::model
