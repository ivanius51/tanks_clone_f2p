#pragma once

#include <cstddef>
#include <cstdint>
#include <memory_resource>

#include "templates/TContainers.h"
#include "templates/TFunction.h"
#include "templates/TTemplateHelper.h"

namespace game::model
{
using FScheduleHandle = TStrongType<std::uint32_t, struct ScheduleHandleTag>;

class Scheduler : public FNonCopyableNonMovable
{
public:
	using Callback = TFunction<void()>;
	using Handle = FScheduleHandle;

	explicit Scheduler(std::pmr::memory_resource* InResource = std::pmr::get_default_resource())
		: Pending(InResource)
	{
	}

	Handle At(double InTime, Callback InCallback);
	Handle After(float InDelay, Callback InCallback);
	bool Cancel(Handle InHandle);
	void Clear();

	void RunDue(double InNow);
	std::size_t GetPendingCount() const;

private:
	struct Entry
	{
		Handle Id;
		double Time = 0.0;
		Callback Action;
	};

	TArray<Entry> Pending;
	std::uint32_t NextId { 1U };
	double LastNow = 0.0;
};
} // namespace game::model