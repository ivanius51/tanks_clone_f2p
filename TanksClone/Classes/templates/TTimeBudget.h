#pragma once

#include <chrono>
#include <utility>

/**
 * Runs InStep repeatedly until it reports that no work is left or InBudget of wall time has passed; at least one step
 * always runs so a tiny budget still makes progress. InStep returns true while more work remains.
 * Returns true when the work is finished. Use it to spread a long build over frames, one small unit per step.
 */
template <typename TStep>
bool RunWithinBudget(const std::chrono::microseconds InBudget, TStep&& InStep)
{
	const auto Start = std::chrono::steady_clock::now();
	do
	{
		if ( !InStep() )
		{
			return true;
		}
	}
	while ( std::chrono::steady_clock::now() - Start < InBudget );
	return false;
}
