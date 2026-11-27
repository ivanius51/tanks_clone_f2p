#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

#include "model/Scheduler.h"
#include "templates/TContainers.h"
#include "templates/TFunction.h"
#include "templates/TTemplateHelper.h"

namespace game::model
{
/**
 * Timers that count their own time from the frame times passed to Tick() (milliseconds), not the simulation clock, so
 * they run while the game is not yet started, like on a loading screen. They are ticked once per frame on the main
 * thread: callbacks and sliced steps run there, as they build engine objects. A job's time budget is wall time.
 * - After(): a callback when a delay has passed.
 * - RunSliced(): a long job given as a step functor; every Update() runs steps until the job's time budget for the
 *   frame is used, then the job continues next frame. The functor returns true while more work remains.
 */
class TimerManager : public FNonCopyableNonMovable
{
public:
	using Handle = FScheduleHandle;
	using Callback = TFunction<void()>;
	using Step = TFunction<bool()>;

	Handle After(float InDelay, Callback InCallback);
	Handle RunSliced(std::chrono::microseconds InBudget, Step InStep, Callback InOnDone = {});
	bool Cancel(Handle InHandle);
	void Clear();

	/** The tickable-manager entry: advances by a frame time in milliseconds; call once per frame from the main thread.
	 * InBudget limits the wall time of the whole tick: timed callbacks first, then the sliced jobs share what is left
	 * (each also keeps its own budget). The first job always runs a step so it cannot starve. */
	void Tick(float InDeltaMs, std::chrono::microseconds InBudget);
	std::size_t GetPendingCount() const;

private:
	struct SlicedJob
	{
		Handle Id;
		std::chrono::microseconds Budget{};
		Step Work;
		Callback OnDone;
	};

	static constexpr std::uint32_t SlicedBit = 0x80000000U;

	Scheduler Timed;
	double Now = 0.0;
	TArray<SlicedJob> Jobs;
	std::uint32_t NextSliced = 1U;
};
} // namespace game::model
