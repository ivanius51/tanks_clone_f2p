#pragma once

#include <cstddef>

#include "services/ResourceHolder.h"
#include "services/ResourceSet.h"

namespace tanks
{

/**
 * Loads a ResourceSet one resource per Step(), so a loading screen can show progress between steps.
 * The loaded resources stay with the job until TakeResources() hands them to the screen that uses them.
 */
class ResourceLoadJob
{
public:
	explicit ResourceLoadJob(const ResourceSet& InSet);

	// Loads the next resource; returns false when nothing is left to load.
	bool Step();
	bool IsFinished() const;
	// 0..1; a job with nothing to load is finished (1).
	float GetProgress() const;
	std::size_t GetFailedCount() const;

	ResourceHolder TakeResources();

private:
	ResourceSet Set;
	std::size_t NextIndex = 0;
	std::size_t FailedCount = 0;
	ResourceHolder Resources;
};

} // namespace tanks
