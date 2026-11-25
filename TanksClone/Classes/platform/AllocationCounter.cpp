#include "platform/AllocationCounter.h"

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <new>

#include "platform/CCPlatformConfig.h"

#include "core/BuildInfo.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
#include <malloc.h>
#include <windows.h>
#endif

namespace game
{

namespace
{
	std::atomic<std::uint64_t> GAllocations{ 0 };
	std::atomic<std::uint64_t> GFrees{ 0 };
	std::atomic<std::uint64_t> GAllocatedBytes{ 0 };
	std::atomic<std::int64_t> GLiveBytes{ 0 };
	std::atomic<std::int64_t> GPeakLiveBytes{ 0 };
	std::atomic<std::int64_t> GPeakBaseline{ 0 };

	void CountAllocation(const std::size_t InSize)
	{
		GAllocations.fetch_add(1, std::memory_order_relaxed);
		GAllocatedBytes.fetch_add(InSize, std::memory_order_relaxed);
		const std::int64_t Live = GLiveBytes.fetch_add(static_cast<std::int64_t>(InSize), std::memory_order_relaxed) + static_cast<std::int64_t>(InSize);
		std::int64_t Peak = GPeakLiveBytes.load(std::memory_order_relaxed);
		while ( Live > Peak && !GPeakLiveBytes.compare_exchange_weak(Peak, Live, std::memory_order_relaxed) )
		{
		}
	}

	void CountFree(const std::size_t InSize)
	{
		GFrees.fetch_add(1, std::memory_order_relaxed);
		GLiveBytes.fetch_sub(static_cast<std::int64_t>(InSize), std::memory_order_relaxed);
	}
} // namespace

bool AllocationCounter::IsEnabled()
{
	return bHasDebugTools;
}

AllocationCounter::Snapshot AllocationCounter::Capture()
{
	Snapshot Result;
	Result.Allocations = GAllocations.load(std::memory_order_relaxed);
	Result.Frees = GFrees.load(std::memory_order_relaxed);
	Result.AllocatedBytes = GAllocatedBytes.load(std::memory_order_relaxed);
	Result.LiveBytes = static_cast<std::uint64_t>(std::max<std::int64_t>(GLiveBytes.load(std::memory_order_relaxed), 0));
	Result.PeakLiveBytes = static_cast<std::uint64_t>(std::max<std::int64_t>(GPeakLiveBytes.load(std::memory_order_relaxed) - GPeakBaseline.load(std::memory_order_relaxed), 0));
	return Result;
}

void AllocationCounter::ResetPeak()
{
	const std::int64_t Live = GLiveBytes.load(std::memory_order_relaxed);
	GPeakBaseline.store(Live, std::memory_order_relaxed);
	GPeakLiveBytes.store(Live, std::memory_order_relaxed);
}

std::size_t AllocationCounter::GetPeakStackBytes()
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	ULONG_PTR Low = 0;
	ULONG_PTR High = 0;
	GetCurrentThreadStackLimits(&Low, &High);
	// The stack is reserved from Low up; pages below the first committed one that is not the guard page were never touched.
	ULONG_PTR Address = Low;
	while ( Address < High )
	{
		MEMORY_BASIC_INFORMATION Info{};
		if ( VirtualQuery(reinterpret_cast<void*>(Address), &Info, sizeof(Info)) == 0 )
		{
			return 0;
		}
		if ( Info.State == MEM_COMMIT && (Info.Protect & PAGE_GUARD) == 0 )
		{
			return static_cast<std::size_t>(High - Address);
		}
		Address = reinterpret_cast<ULONG_PTR>(Info.BaseAddress) + Info.RegionSize;
	}
#endif
	return 0;
}

} // namespace game

// The global replacements exist only where the counter is on (Debug and Test); the Release build keeps the CRT ones.
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) && (!defined(NDEBUG) || defined(TANKS_DEBUG_TOOLS))

namespace
{
	void* AllocateCounted(const std::size_t InSize)
	{
		void* const Block = std::malloc(InSize != 0 ? InSize : 1);
		if ( Block )
		{
			game::CountAllocation(_msize(Block));
		}
		return Block;
	}

	void* AllocateCountedAligned(const std::size_t InSize, const std::align_val_t InAlignment)
	{
		const std::size_t Alignment = static_cast<std::size_t>(InAlignment);
		void* const Block = _aligned_malloc(InSize != 0 ? InSize : 1, Alignment);
		if ( Block )
		{
			game::CountAllocation(_aligned_msize(Block, Alignment, 0));
		}
		return Block;
	}

	void FreeCounted(void* const InBlock)
	{
		if ( !InBlock )
		{
			return;
		}
		game::CountFree(_msize(InBlock));
		std::free(InBlock);
	}

	void FreeCountedAligned(void* const InBlock, const std::align_val_t InAlignment)
	{
		if ( !InBlock )
		{
			return;
		}
		game::CountFree(_aligned_msize(InBlock, static_cast<std::size_t>(InAlignment), 0));
		_aligned_free(InBlock);
	}
}

void* operator new(const std::size_t InSize)
{
	if ( void* const Block = AllocateCounted(InSize) )
	{
		return Block;
	}
	throw std::bad_alloc();
}

void* operator new[](const std::size_t InSize)
{
	if ( void* const Block = AllocateCounted(InSize) )
	{
		return Block;
	}
	throw std::bad_alloc();
}

void* operator new(const std::size_t InSize, const std::nothrow_t&) noexcept
{
	return AllocateCounted(InSize);
}

void* operator new[](const std::size_t InSize, const std::nothrow_t&) noexcept
{
	return AllocateCounted(InSize);
}

void* operator new(const std::size_t InSize, const std::align_val_t InAlignment)
{
	if ( void* const Block = AllocateCountedAligned(InSize, InAlignment) )
	{
		return Block;
	}
	throw std::bad_alloc();
}

void* operator new[](const std::size_t InSize, const std::align_val_t InAlignment)
{
	if ( void* const Block = AllocateCountedAligned(InSize, InAlignment) )
	{
		return Block;
	}
	throw std::bad_alloc();
}

void* operator new(const std::size_t InSize, const std::align_val_t InAlignment, const std::nothrow_t&) noexcept
{
	return AllocateCountedAligned(InSize, InAlignment);
}

void* operator new[](const std::size_t InSize, const std::align_val_t InAlignment, const std::nothrow_t&) noexcept
{
	return AllocateCountedAligned(InSize, InAlignment);
}

void operator delete(void* const InBlock) noexcept
{
	FreeCounted(InBlock);
}

void operator delete[](void* const InBlock) noexcept
{
	FreeCounted(InBlock);
}

void operator delete(void* const InBlock, const std::size_t) noexcept
{
	FreeCounted(InBlock);
}

void operator delete[](void* const InBlock, const std::size_t) noexcept
{
	FreeCounted(InBlock);
}

void operator delete(void* const InBlock, const std::nothrow_t&) noexcept
{
	FreeCounted(InBlock);
}

void operator delete[](void* const InBlock, const std::nothrow_t&) noexcept
{
	FreeCounted(InBlock);
}

void operator delete(void* const InBlock, const std::align_val_t InAlignment) noexcept
{
	FreeCountedAligned(InBlock, InAlignment);
}

void operator delete[](void* const InBlock, const std::align_val_t InAlignment) noexcept
{
	FreeCountedAligned(InBlock, InAlignment);
}

void operator delete(void* const InBlock, const std::size_t, const std::align_val_t InAlignment) noexcept
{
	FreeCountedAligned(InBlock, InAlignment);
}

void operator delete[](void* const InBlock, const std::size_t, const std::align_val_t InAlignment) noexcept
{
	FreeCountedAligned(InBlock, InAlignment);
}

void operator delete(void* const InBlock, const std::align_val_t InAlignment, const std::nothrow_t&) noexcept
{
	FreeCountedAligned(InBlock, InAlignment);
}

void operator delete[](void* const InBlock, const std::align_val_t InAlignment, const std::nothrow_t&) noexcept
{
	FreeCountedAligned(InBlock, InAlignment);
}

#endif
