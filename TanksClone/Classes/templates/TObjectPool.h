#pragma once

#include <cstddef>
#include <vector>

/**
 * A fixed-size pool of reusable objects. Acquire hands out a free slot, Release returns it; no allocation after construction.
 */
template <typename T>
class TObjectPool
{
public:
	explicit TObjectPool(const std::size_t InCapacity)
		: Slots(InCapacity)
	{
		FreeSlots.reserve(InCapacity);
		for (std::size_t Index = InCapacity; Index > 0; --Index)
		{
			FreeSlots.push_back(Index - 1);
		}
	}

	T* Acquire()
	{
		if (FreeSlots.empty())
		{
			return nullptr;
		}
		const std::size_t Index = FreeSlots.back();
		FreeSlots.pop_back();
		return &Slots[Index];
	}

	bool Release(T* const InObject)
	{
		if (InObject == nullptr || InObject < Slots.data() || InObject >= Slots.data() + Slots.size())
		{
			return false;
		}
		FreeSlots.push_back(static_cast<std::size_t>(InObject - Slots.data()));
		return true;
	}

	std::size_t Capacity() const noexcept { return Slots.size(); }
	std::size_t FreeCount() const noexcept { return FreeSlots.size(); }

private:
	std::vector<T> Slots;
	std::vector<std::size_t> FreeSlots;
};
