#pragma once

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

#include "templates/TFunction.h"

namespace game::model
{
/** Which jobs the worker takes first and the main thread delivers first. Within one priority: submit order. */
enum class WorkPriority
{
	High,
	Normal,
	Low
};

/**
 * One worker thread that waits for jobs, and a queue of finished jobs that the main thread drains with Pump()
 * (once per frame). A job is two parts: Work runs on the worker thread and must not touch the engine or any
 * shared game state (file reads, decoding, parsing); Done runs inside Pump() on the main thread and gets the result,
 * so it may use the engine.
 *
 * To load a new kind of resource in the background (a binary config, a level, a texture) write a Work function that
 * returns the parsed data and a Done function that turns it into the real object; nothing else changes.
 *
 * During gameplay give Pump() a time budget so a burst of finished jobs is spread over several frames, and call
 * CancelAll() when what the jobs were for is gone (a level change).
 */
class WorkQueue
{
public:
	WorkQueue() = default;
	~WorkQueue();
	WorkQueue(const WorkQueue&) = delete;
	WorkQueue& operator=(const WorkQueue&) = delete;

	template <typename TResult>
	void Submit(TFunction<TResult()> InWork, TFunction<void(TResult)> InDone, const WorkPriority InPriority = WorkPriority::Normal)
	{
		const auto Result = MakeSharedValue<TResult>();
		Enqueue(
			[Work = std::move(InWork), Result]() { Result->Get() = Work(); },
			[Done = std::move(InDone), Result]() { Done(std::move(Result->Get())); },
			InPriority);
	}

	std::size_t Pump(std::chrono::microseconds InBudget = std::chrono::microseconds::max());
	std::size_t GetPendingCount() const;

	void CancelAll();
	void WaitIdle();

private:
	static constexpr std::size_t TaskInlineSize = 320;
	using Task = TFunction<void(), TaskInlineSize>;

	struct Job
	{
		Task Work;
		Task Done;
	};

	static constexpr std::size_t PriorityCount = 3;
	using Lanes = std::array<std::deque<Job>, PriorityCount>;

	void Enqueue(Task InWork, Task InDone, WorkPriority InPriority);
	void Run();
	bool PopWaiting(Job& OutJob, std::size_t& OutLane);

	mutable std::mutex Mutex;
	std::condition_variable Wake;
	std::condition_variable Idle;
	Lanes Waiting;
	Lanes Finished;
	std::size_t PendingCount = 0;
	std::size_t Generation = 0;
	bool bRunning = false;
	bool bStopping = false;
	std::thread Worker;
};
} // namespace game::model
