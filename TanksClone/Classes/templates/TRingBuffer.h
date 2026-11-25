#pragma once

#include <array>
#include <cstddef>
#include <utility>

/**
 * A fixed-capacity FIFO over inline storage. Push returns false when full instead of allocating.
 */
template <typename T, std::size_t TCapacity>
class TRingBuffer
{
	static_assert(TCapacity > 0, "TRingBuffer: capacity must be positive");

public:
	constexpr bool Push(const T& InValue)
	{
		if (IsFull())
		{
			return false;
		}
		Items[(Head + Count) % TCapacity] = InValue;
		++Count;
		return true;
	}

	constexpr bool Pop(T& OutValue)
	{
		if (IsEmpty())
		{
			return false;
		}
		OutValue = std::move(Items[Head]);
		Head = (Head + 1) % TCapacity;
		--Count;
		return true;
	}

	constexpr void Clear() noexcept
	{
		Head = 0;
		Count = 0;
	}

	constexpr std::size_t Size() const noexcept { return Count; }
	constexpr std::size_t Capacity() const noexcept { return TCapacity; }
	constexpr bool IsEmpty() const noexcept { return Count == 0; }
	constexpr bool IsFull() const noexcept { return Count == TCapacity; }

private:
	std::array<T, TCapacity> Items{};
	std::size_t Head = 0;
	std::size_t Count = 0;
};
