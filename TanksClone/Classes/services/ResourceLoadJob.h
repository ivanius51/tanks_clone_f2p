#pragma once

#include <cstddef>
#include <memory>

#include "services/ResourceHolder.h"
#include "services/ResourceSet.h"

namespace game
{
/**
 * Loads a ResourceSet so a loading screen can show progress between calls of Step().
 * Synchronous: one resource per Step(). Asynchronous: the first Step() asks for every atlas (read and decoded on
 * the loader thread) and every sound (loaded by the audio engine) at once; the job finishes when the last one
 * arrives, which needs the engine's main loop to run between steps.
 * The loaded resources stay with the job until TakeResources() hands them to the screen that uses them.
 */
class ResourceLoadJob
{
public:
	ResourceLoadJob(const ResourceSet& InSet, bool bInAsync);

	bool Step();
	bool IsFinished() const;
	float GetProgress() const;
	std::size_t GetFailedCount() const;

	ResourceHolder TakeResources();

private:
	/** Shared with the background requests, which may outlive the job. */
	struct State
	{
		ResourceHolder Resources;
		std::size_t DoneCount = 0;
		std::size_t FailedCount = 0;
	};

	void StepSync();
	void StepAsync();

	ResourceSet Set;
	bool bAsync;
	bool bRequested = false;
	std::shared_ptr<State> Shared = std::make_shared<State>();
	std::size_t NextIndex = 0;
};
} // namespace game
