#pragma once

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <utility>

namespace tanks::model
{

/** Which jobs the worker takes first and the main thread delivers first. Within one priority: submit order. */
enum class WorkPriority
{
	High,   // needed by the next frames, e.g. the map chunk the camera moves into
	Normal,
	Low     // background, e.g. a tileset for a distant part of the map
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

	// Starts the worker thread on first use.
	template <typename TResult>
	void Submit(std::function<TResult()> InWork, std::function<void(TResult)> InDone, const WorkPriority InPriority = WorkPriority::Normal)
	{
		auto Result = std::make_shared<TResult>();
		Enqueue(
			[Work = std::move(InWork), Result]() { *Result = Work(); },
			[Done = std::move(InDone), Result]() { Done(std::move(*Result)); },
			InPriority);
	}

	// Runs the Done part of finished jobs, most urgent first; returns how many ran. Main thread only.
	// At least one job runs per call; after that it stops once InBudget is spent (the rest wait for the next call).
	std::size_t Pump(std::chrono::microseconds InBudget = std::chrono::microseconds::max());
	// Jobs submitted and not yet delivered.
	std::size_t GetPendingCount() const;

	// Drops every job that has not been delivered: waiting ones never run, finished ones are never delivered, and a job
	// that is running now is dropped when it ends. Their Done functions are not called.
	void CancelAll();
	// Blocks until the worker has nothing waiting and nothing running (finished jobs may still need Pump()).
	void WaitIdle();

private:
	struct Job
	{
		std::function<void()> Work;
		std::function<void()> Done;
	};

	static constexpr std::size_t PriorityCount = 3;
	using Lanes = std::array<std::deque<Job>, PriorityCount>;

	void Enqueue(std::function<void()> InWork, std::function<void()> InDone, WorkPriority InPriority);
	void Run();
	bool PopWaiting(Job& OutJob, std::size_t& OutLane);

	mutable std::mutex Mutex;
	std::condition_variable Wake;
	std::condition_variable Idle;
	Lanes Waiting;
	Lanes Finished;
	std::size_t PendingCount = 0;
	std::size_t Generation = 0; // raised by CancelAll(); a running job from an older generation is dropped
	bool bRunning = false;
	bool bStopping = false;
	std::thread Worker;
};

} // namespace tanks::model
