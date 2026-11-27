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
 * Real-time timers that keep their own time (the monotonic system clock), not the simulation clock, so they keep
 * running while the game is paused or not yet started, like on a loading screen. The platform calls Update() once per
 * frame on the main thread: callbacks and sliced steps run there, as they build engine objects.
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

	/** Advances to the real time now (monotonic system clock); call once per frame from the main thread. */
	void Update();
	/** Advances to an explicit time in seconds since start; the same as Update() but for tests. */
	void Update(double InNow);
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
	std::chrono::steady_clock::time_point Start = std::chrono::steady_clock::now();
	TArray<SlicedJob> Jobs;
	std::uint32_t NextSliced = 1U;
};
} // namespace game::model
