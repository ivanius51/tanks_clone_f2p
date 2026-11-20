#include "model/WorkQueue.h"

namespace tanks::model
{

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

void WorkQueue::Enqueue(std::function<void()> InWork, std::function<void()> InDone)
{
	{
		std::lock_guard<std::mutex> Lock(Mutex);
		Waiting.push_back(Job{ std::move(InWork), std::move(InDone) });
		++PendingCount;
		if ( !Worker.joinable() )
		{
			Worker = std::thread(&WorkQueue::Run, this);
		}
	}
	Wake.notify_one();
}

void WorkQueue::Run()
{
	while ( true )
	{
		Job Current;
		{
			std::unique_lock<std::mutex> Lock(Mutex);
			Wake.wait(Lock, [this]() { return bStopping || !Waiting.empty(); });
			if ( Waiting.empty() )
			{
				return;
			}
			Current = std::move(Waiting.front());
			Waiting.pop_front();
		}

		Current.Work();

		std::lock_guard<std::mutex> Lock(Mutex);
		Finished.push_back(std::move(Current.Done));
	}
}

std::size_t WorkQueue::Pump()
{
	std::deque<std::function<void()>> Ready;
	{
		std::lock_guard<std::mutex> Lock(Mutex);
		Ready.swap(Finished);
	}
	for ( const std::function<void()>& CurrentDone : Ready )
	{
		CurrentDone();
	}
	{
		std::lock_guard<std::mutex> Lock(Mutex);
		PendingCount -= Ready.size();
	}
	return Ready.size();
}

std::size_t WorkQueue::GetPendingCount() const
{
	std::lock_guard<std::mutex> Lock(Mutex);
	return PendingCount;
}

} // namespace tanks::model
