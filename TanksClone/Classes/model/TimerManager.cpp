#include "model/TimerManager.h"

#include <algorithm>
#include <utility>

#include "templates/TTimeBudget.h"

namespace game::model
{

TimerManager::Handle TimerManager::After(const float InDelay, Callback InCallback)
{
	return Timed.At(Now + std::max(0.0f, InDelay), std::move(InCallback));
}

TimerManager::Handle TimerManager::RunSliced(const std::chrono::microseconds InBudget, Step InStep, Callback InOnDone)
{
	const Handle Id { SlicedBit | NextSliced++ };
	Jobs.PushBack({ Id, InBudget, std::move(InStep), std::move(InOnDone) });
	return Id;
}

bool TimerManager::Cancel(const Handle InHandle)
{
	if ( (InHandle.Value & SlicedBit) == 0U )
	{
		return Timed.Cancel(InHandle);
	}
	const auto Found = std::find_if(Jobs.begin(), Jobs.end(), [InHandle](const SlicedJob& InJob) { return InJob.Id == InHandle; });
	if ( Found == Jobs.end() )
	{
		return false;
	}
	Jobs.Erase(Found);
	return true;
}

void TimerManager::Clear()
{
	Timed.Clear();
	Jobs.Clear();
}

void TimerManager::Tick(const float InDeltaMs)
{
	if ( InDeltaMs > 0.0f )
	{
		Now += static_cast<double>(InDeltaMs / 1000.0f);
	}
	Timed.RunDue(Now);
	// Index-based: a callback may add or cancel jobs.
	for ( std::size_t Index = 0; Index < Jobs.Size(); )
	{
		const Handle Id = Jobs[Index].Id;
		const std::chrono::microseconds Budget = Jobs[Index].Budget;
		const bool bFinished = RunWithinBudget(Budget, [this, Id]()
		{
			const auto Found = std::find_if(Jobs.begin(), Jobs.end(), [Id](const SlicedJob& InJob) { return InJob.Id == Id; });
			return Found != Jobs.end() && Found->Work();
		});
		const auto Found = std::find_if(Jobs.begin(), Jobs.end(), [Id](const SlicedJob& InJob) { return InJob.Id == Id; });
		if ( Found == Jobs.end() )
		{
			continue;
		}
		if ( !bFinished )
		{
			++Index;
			continue;
		}
		Callback Done = std::move(Found->OnDone);
		Jobs.Erase(Found);
		if ( Done )
		{
			Done();
		}
	}
}

std::size_t TimerManager::GetPendingCount() const
{
	return Timed.GetPendingCount() + Jobs.Size();
}

} // namespace game::model
