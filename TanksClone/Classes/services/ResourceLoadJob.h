#pragma once

#include <cstddef>
#include <memory>

#include "services/ResourceHolder.h"
#include "services/ResourceSet.h"

namespace tanks
{

/**
 * Loads a ResourceSet so a loading screen can show progress between calls of Step().
 * Synchronous: one resource per Step(). Asynchronous: the first Step() starts decoding every atlas in the
 * background, and each Step() then loads one sound; the job finishes when the last atlas arrives, which needs
 * the engine's main loop to run between steps.
 * The loaded resources stay with the job until TakeResources() hands them to the screen that uses them.
 */
class ResourceLoadJob
{
public:
	ResourceLoadJob(const ResourceSet& InSet, bool bInAsync);

	// Advances the load; returns false once the job is finished.
	bool Step();
	bool IsFinished() const;
	// 0..1; a job with nothing to load is finished (1).
	float GetProgress() const;
	std::size_t GetFailedCount() const;

	ResourceHolder TakeResources();

private:
	// Shared with the background requests, which may outlive the job.
	struct State
	{
		ResourceHolder Resources;
		std::size_t DoneCount = 0;
		std::size_t FailedCount = 0;
	};

	void StepSync();
	void StepAsync();
	void LoadSound(const std::string& InSound);

	ResourceSet Set;
	bool bAsync;
	bool bAtlasesRequested = false;
	std::size_t NextSound = 0;
	std::shared_ptr<State> Shared = std::make_shared<State>();
	std::size_t NextIndex = 0;
};

} // namespace tanks
