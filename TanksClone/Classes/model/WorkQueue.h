#pragma once

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

/**
 * One worker thread that waits for jobs, and a queue of finished jobs that the main thread drains with Pump()
 * (once per frame). A job is two parts: Work runs on the worker thread and must not touch the engine or any
 * shared game state (file reads, decoding, parsing); Done runs inside Pump() on the main thread and gets the result,
 * so it may use the engine. Jobs run in the order they were submitted.
 *
 * To load a new kind of resource in the background (a binary config, a level, a texture) write a Work function that
 * returns the parsed data and a Done function that turns it into the real object; nothing else changes.
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
	void Submit(std::function<TResult()> InWork, std::function<void(TResult)> InDone)
	{
		auto Result = std::make_shared<TResult>();
		Enqueue(
			[Work = std::move(InWork), Result]() { *Result = Work(); },
			[Done = std::move(InDone), Result]() { Done(std::move(*Result)); });
	}

	// Runs the Done part of every finished job; returns how many ran. Main thread only.
	std::size_t Pump();
	// Jobs submitted and not yet pumped.
	std::size_t GetPendingCount() const;

private:
	struct Job
	{
		std::function<void()> Work;
		std::function<void()> Done;
	};

	void Enqueue(std::function<void()> InWork, std::function<void()> InDone);
	void Run();

	mutable std::mutex Mutex;
	std::condition_variable Wake;
	std::deque<Job> Waiting;
	std::deque<std::function<void()>> Finished;
	std::size_t PendingCount = 0;
	bool bStopping = false;
	std::thread Worker;
};

} // namespace tanks::model
