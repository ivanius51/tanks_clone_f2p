#include "model/Scheduler.h"

#include <algorithm>
#include <utility>

namespace game::model
{

Scheduler::Handle Scheduler::At(const double InTime, Callback InCallback)
{
	const Handle Id { NextId++ };
	const auto Position = std::upper_bound(Pending.begin(), Pending.end(), InTime, [](const double InValue, const Entry& InEntry)
	{
		return InValue < InEntry.Time;
	});
	Pending.Insert(Position, { Id, InTime, std::move(InCallback) });
	return Id;
}

Scheduler::Handle Scheduler::After(const float InDelay, Callback InCallback)
{
	return At(LastNow + std::max(0.0f, InDelay), std::move(InCallback));
}

bool Scheduler::Cancel(const Handle InHandle)
{
	const auto Found = std::find_if(Pending.begin(), Pending.end(), [InHandle](const Entry& InEntry) { return InEntry.Id == InHandle; });
	if ( Found == Pending.end() )
	{
		return false;
	}
	Pending.Erase(Found);
	return true;
}

void Scheduler::Clear()
{
	Pending.Clear();
}

void Scheduler::RunDue(const double InNow)
{
	LastNow = InNow;
	while ( !Pending.IsEmpty() && Pending.AsVector().front().Time <= InNow )
	{
		Callback Action = std::move(Pending.AsVector().front().Action);
		Pending.Erase(Pending.begin());
		Action();
	}
}

std::size_t Scheduler::GetPendingCount() const
{
	return Pending.Size();
}

} // namespace game::model
