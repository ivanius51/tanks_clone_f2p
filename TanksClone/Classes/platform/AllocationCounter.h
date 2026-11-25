#pragma once

#include <cstddef>
#include <cstdint>

namespace game
{

/**
 * Counts the heap allocations of the whole process (a replacement of the global operator new and delete) and
 * reads the peak stack depth of the calling thread. Exists only in the Debug and Test builds; in Release
 * IsEnabled() is false and every number is zero. The benchmark test logs it to show what a refactoring saves.
 */
class AllocationCounter
{
public:
	struct Snapshot
	{
		std::uint64_t Allocations = 0;
		std::uint64_t Frees = 0;
		std::uint64_t AllocatedBytes = 0;
		std::uint64_t LiveBytes = 0;
		std::uint64_t PeakLiveBytes = 0;
	};

	static bool IsEnabled();
	static Snapshot Capture();
	static void ResetPeak();

	// Peak live bytes in a Snapshot is the growth since ResetPeak(). The deepest the stack of the calling thread has grown so far (committed pages), in bytes.
	static std::size_t GetPeakStackBytes();
};

} // namespace game
