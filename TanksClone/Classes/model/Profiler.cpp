#include "model/Profiler.h"

#include <algorithm>
#include <cstdio>
#include <mutex>
#include <string_view>

#include "templates/TContainers.h"

namespace game::model
{
namespace
{
	struct Entry
	{
		std::string_view Name;
		long long Count = 0;
		long long TotalNs = 0;
		long long WorstNs = 0;
	};

	std::mutex GLock;
	TArray<Entry> GEntries;
}

void Profiler::Record(const char* const InName, const std::chrono::nanoseconds InElapsed)
{
	const long long Ns = InElapsed.count();
	const std::lock_guard<std::mutex> Guard(GLock);
	const std::string_view Name(InName);
	auto Found = std::find_if(GEntries.begin(), GEntries.end(), [Name](const Entry& InEntry) { return InEntry.Name == Name; });
	if ( Found == GEntries.end() )
	{
		GEntries.PushBack(Entry{ Name, 0, 0, 0 });
		Found = GEntries.end() - 1;
	}
	++Found->Count;
	Found->TotalNs += Ns;
	Found->WorstNs = std::max(Found->WorstNs, Ns);
}

void Profiler::Reset()
{
	const std::lock_guard<std::mutex> Guard(GLock);
	GEntries.Clear();
}

std::string Profiler::BuildReport()
{
	TArray<Entry> Sorted;
	{
		const std::lock_guard<std::mutex> Guard(GLock);
		for ( const Entry& Current : GEntries )
		{
			Sorted.PushBack(Current);
		}
	}
	std::sort(Sorted.begin(), Sorted.end(), [](const Entry& InA, const Entry& InB) { return InA.TotalNs > InB.TotalNs; });
	std::string Report;
	for ( const Entry& Current : Sorted )
	{
		char Line[256];
		std::snprintf(Line, sizeof(Line), "  %-28.*s calls %6lld  total %9.2f ms  avg %8.3f ms  worst %8.3f ms\n",
			static_cast<int>(Current.Name.size()), Current.Name.data(), Current.Count,
			static_cast<double>(Current.TotalNs) / 1e6, static_cast<double>(Current.TotalNs) / 1e6 / static_cast<double>(std::max(1LL, Current.Count)),
			static_cast<double>(Current.WorstNs) / 1e6);
		Report += Line;
	}
	return Report;
}
} // namespace game::model
