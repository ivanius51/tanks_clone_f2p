#include "model/WorkQueue.h"

#include <algorithm>

namespace tanks::model
{

namespace
{
	std::size_t LaneOf(const WorkPriority InPriority)
	{
		return static_cast<std::size_t>(InPriority);
	}
}

WorkQueue::~WorkQueue()
{
	{
		std::lock_guard<std::mutex> Lock(Mutex);
		bStopping = true;
	}
	Wake.notify_all();
	if ( Worker.joinable() )
	{
		Worker.join();
	}
}

void WorkQueue::Enqueue(std::function<void()> InWork, std::function<void()> InDone, const WorkPriority InPriority)
{
	{
		std::lock_guard<std::mutex> Lock(Mutex);
		Waiting[LaneOf(InPriority)].push_back(Job{ std::move(InWork), std::move(InDone) });
		++PendingCount;
		if ( !Worker.joinable() )
		{
			Worker = std::thread(&WorkQueue::Run, this);
		}
	}
	Wake.notify_one();
}

bool WorkQueue::PopWaiting(Job& OutJob, std::size_t& OutLane)
{
	for ( std::size_t Lane = 0; Lane < Waiting.size(); ++Lane )
	{
		if ( !Waiting[Lane].empty() )
		{
			OutJob = std::move(Waiting[Lane].front());
			Waiting[Lane].pop_front();
			OutLane = Lane;
			return true;
		}
	}
	return false;
}

void WorkQueue::Run()
{
	while ( true )
	{
		Job Current;
		std::size_t StartedIn = 0;
		std::size_t Lane = 0;
		{
			std::unique_lock<std::mutex> Lock(Mutex);
			Wake.wait(Lock, [this]()
			{
				return bStopping || std::any_of(Waiting.begin(), Waiting.end(), [](const std::deque<Job>& InLane) { return !InLane.empty(); });
			});
			if ( !PopWaiting(Current, Lane) )
			{
				return;
			}
			bRunning = true;
			StartedIn = Generation;
		}

		Current.Work();

		std::lock_guard<std::mutex> Lock(Mutex);
		bRunning = false;
		if ( StartedIn == Generation )
		{
			Finished[Lane].push_back(std::move(Current));
		}
		else
		{
			--PendingCount;
		}
		Idle.notify_all();
	}
}

std::size_t WorkQueue::Pump(const std::chrono::microseconds InBudget)
{
	const auto Start = std::chrono::steady_clock::now();
	std::size_t Delivered = 0;
	while ( true )
	{
		Job Ready;
		{
			std::lock_guard<std::mutex> Lock(Mutex);
			bool bFound = false;
			for ( std::deque<Job>& Lane : Finished )
			{
				if ( !Lane.empty() )
				{
					Ready = std::move(Lane.front());
					Lane.pop_front();
					bFound = true;
					break;
				}
			}
			if ( !bFound )
			{
				break;
			}
		}

		Ready.Done();
		++Delivered;
		{
			std::lock_guard<std::mutex> Lock(Mutex);
			if ( PendingCount > 0 )
			{
				--PendingCount;
			}
		}
		if ( InBudget != std::chrono::microseconds::max() && std::chrono::steady_clock::now() - Start >= InBudget )
		{
			break;
		}
	}
	return Delivered;
}

std::size_t WorkQueue::GetPendingCount() const
{
	std::lock_guard<std::mutex> Lock(Mutex);
	return PendingCount;
}

void WorkQueue::CancelAll()
{
	std::lock_guard<std::mutex> Lock(Mutex);
	++Generation;
	for ( Lanes* const Group : { &Waiting, &Finished } )
	{
		for ( std::deque<Job>& Lane : *Group )
		{
			PendingCount -= std::min(PendingCount, Lane.size());
			Lane.clear();
		}
	}
}

void WorkQueue::WaitIdle()
{
	std::unique_lock<std::mutex> Lock(Mutex);
	Idle.wait(Lock, [this]()
	{
		return !bRunning && std::all_of(Waiting.begin(), Waiting.end(), [](const std::deque<Job>& InLane) { return InLane.empty(); });
	});
}

} // namespace tanks::model
