#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

/**
 * A handle to a slot in TSlotMap: the index plus a generation, so a stale handle to a reused slot is rejected.
 */
struct TEntityHandle
{
	std::uint32_t Index = 0;
	std::uint32_t Generation = 0;

	constexpr bool IsValid() const noexcept { return Generation != 0; }
	constexpr bool operator==(const TEntityHandle InOther) const noexcept
	{
		return Index == InOther.Index && Generation == InOther.Generation;
	}
	constexpr bool operator!=(const TEntityHandle InOther) const noexcept { return !(*this == InOther); }
};

/**
 * Storage of items addressed by TEntityHandle. Removing an item frees its slot for reuse; old handles stop resolving.
 */
template <typename T>
class TSlotMap
{
public:
	TEntityHandle Add(const T& InValue)
	{
		std::uint32_t Index = 0;
		if (!FreeIndices.empty())
		{
			Index = FreeIndices.back();
			FreeIndices.pop_back();
			Slots[Index].Value = InValue;
			Slots[Index].bIsOccupied = true;
		}
		else
		{
			Index = static_cast<std::uint32_t>(Slots.size());
			Slots.push_back(Slot{InValue, 1, true});
		}
		++Count;
		return TEntityHandle{Index, Slots[Index].Generation};
	}

	bool Remove(const TEntityHandle InHandle)
	{
		if (!IsAlive(InHandle))
		{
			return false;
		}
		Slot& Target = Slots[InHandle.Index];
		Target.bIsOccupied = false;
		++Target.Generation;
		FreeIndices.push_back(InHandle.Index);
		--Count;
		return true;
	}

	T* Find(const TEntityHandle InHandle)
	{
		return IsAlive(InHandle) ? &Slots[InHandle.Index].Value : nullptr;
	}

	const T* Find(const TEntityHandle InHandle) const
	{
		return IsAlive(InHandle) ? &Slots[InHandle.Index].Value : nullptr;
	}

	bool IsAlive(const TEntityHandle InHandle) const
	{
		return InHandle.Index < Slots.size() && Slots[InHandle.Index].bIsOccupied && Slots[InHandle.Index].Generation == InHandle.Generation;
	}

	std::size_t Size() const noexcept { return Count; }

private:
	struct Slot
	{
		T Value{};
		std::uint32_t Generation = 1;
		bool bIsOccupied = false;
	};

	std::vector<Slot> Slots;
	std::vector<std::uint32_t> FreeIndices;
	std::size_t Count = 0;
};
