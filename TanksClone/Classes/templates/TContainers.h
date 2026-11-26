#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <memory_resource>
#include <new>
#include <utility>
#include <type_traits>
#include <vector>

#include "templates/TStoredString.h"


using SizeType = std::size_t;

template <typename T, typename TAllocatorType = std::pmr::polymorphic_allocator<T>>
class TArray
{
	static_assert(!std::is_same_v<T, bool>, "vector<bool> returns proxies; use TArray<uint8_t>");
public:
	using VectorType = std::vector<T, TAllocatorType>;
	using iterator = typename VectorType::iterator;
	using const_iterator = typename VectorType::const_iterator;
	using value_type = T;
	using reference = T&;
	using const_reference = const T&;
	using size_type = SizeType;
	using Iterator = iterator;
	using ConstIterator = const_iterator;

	TArray() noexcept = default;

	explicit TArray(const TAllocatorType& InAlloc) noexcept
		: Data(InAlloc)
	{}

	explicit TArray(std::pmr::memory_resource* InResource) noexcept
		: Data(InResource)
	{}

	TArray(SizeType InCount, const T& InValue, const TAllocatorType& InAlloc = {})
		: Data(InCount, InValue, InAlloc)
	{}

	TArray(std::initializer_list<T> InInit, const TAllocatorType& InAlloc = {})
		: Data(InInit, InAlloc)
	{}

	explicit TArray(SizeType InCount)
		: Data(InCount)
	{}

	template <typename TInputIt, typename = typename std::iterator_traits<TInputIt>::iterator_category>
	TArray(TInputIt InFirst, TInputIt InLast, const TAllocatorType& InAlloc = {})
		: Data(InFirst, InLast, InAlloc)
	{}

	friend bool operator==(const TArray& InLeft, const TArray& InRight) { return InLeft.Data == InRight.Data; }
	friend bool operator!=(const TArray& InLeft, const TArray& InRight) { return !(InLeft.Data == InRight.Data); }

	template <typename TInputIt>
	void Assign(TInputIt InFirst, TInputIt InLast) { Data.assign(InFirst, InLast); }
	void Assign(SizeType InCount, const T& InValue) { Data.assign(InCount, InValue); }
	void Swap(TArray& InOther) noexcept { Data.swap(InOther.Data); }
	void ShrinkToFit() { Data.shrink_to_fit(); }

	std::reverse_iterator<Iterator> rbegin() noexcept { return Data.rbegin(); }
	std::reverse_iterator<Iterator> rend() noexcept { return Data.rend(); }
	std::reverse_iterator<ConstIterator> rbegin() const noexcept { return Data.rbegin(); }
	std::reverse_iterator<ConstIterator> rend() const noexcept { return Data.rend(); }
	// --- PascalCase API ---
	SizeType Size() const noexcept { return Data.size(); }
	SizeType Capacity() const noexcept { return Data.capacity(); }
	bool IsEmpty() const noexcept { return Data.empty(); }

	void Reserve(SizeType InNewCap) { Data.reserve(InNewCap); }
	void Resize(SizeType InCount) { Data.resize(InCount); }
	void Resize(SizeType InCount, const T& InValue) { Data.resize(InCount, InValue); }
	void Clear() noexcept { Data.clear(); }

	void PushBack(const T& InValue) { Data.push_back(InValue); }
	void PushBack(T&& InValue) { Data.push_back(std::move(InValue)); }
	void PopBack() { Data.pop_back(); }

	template <typename... TArgs>
	decltype(auto) EmplaceBack(TArgs&&... InArgs)
	{
		return Data.emplace_back(std::forward<TArgs>(InArgs)...);
	}

	T& operator[](SizeType InIndex) noexcept { return Data[InIndex]; }
	const T& operator[](SizeType InIndex) const noexcept { return Data[InIndex]; }

	T* GetData() noexcept { return Data.data(); }
	const T* GetData() const noexcept { return Data.data(); }

	T& Front() noexcept { return Data.front(); }
	const T& Front() const noexcept { return Data.front(); }
	T& Back() noexcept { return Data.back(); }
	const T& Back() const noexcept { return Data.back(); }

	Iterator begin() noexcept { return Data.begin(); }
	ConstIterator begin() const noexcept { return Data.begin(); }
	ConstIterator Cbegin() const noexcept { return Data.cbegin(); }

	Iterator end() noexcept { return Data.end(); }
	ConstIterator end() const noexcept { return Data.end(); }
	ConstIterator Cend() const noexcept { return Data.cend(); }

	TAllocatorType GetAllocator() const noexcept { return Data.get_allocator(); }
	VectorType& AsVector() noexcept { return Data; }
	const VectorType& AsVector() const noexcept { return Data; }

	Iterator Insert(ConstIterator InPos, const T& InValue)
	{
		return Data.insert(InPos, InValue);
	}

	Iterator Insert(ConstIterator InPos, T&& InValue)
	{
		return Data.insert(InPos, std::move(InValue));
	}

	Iterator Insert(ConstIterator InPos, SizeType InCount, const T& InValue)
	{
		return Data.insert(InPos, InCount, InValue);
	}

	template <typename TInputIt>
	Iterator Insert(ConstIterator InPos, TInputIt InFirst, TInputIt InLast)
	{
		return Data.insert(InPos, InFirst, InLast);
	}

	template <typename... TArgs>
	Iterator Emplace(ConstIterator InPos, TArgs&&... InArgs)
	{
		return Data.emplace(InPos, std::forward<TArgs>(InArgs)...);
	}

	void RemoveIf(std::function<bool(const T&)> InPredicate)
	{
		Data.erase(std::remove_if(Data.begin(), Data.end(), InPredicate), Data.end());
	}

	Iterator Erase(ConstIterator InPos)
	{
		return Data.erase(InPos);
	}

	Iterator Erase(ConstIterator InFirst, ConstIterator InLast)
	{
		return Data.erase(InFirst, InLast);
	}

	void RemoveAt(SizeType InIndex)
	{
		Data.erase(Data.begin() + static_cast<typename VectorType::difference_type>(InIndex));
	}

	void RemoveAtSwap(SizeType InIndex)
	{
		const SizeType LastIndex = Data.size() - 1;
		if (InIndex != LastIndex)
		{
			Data[InIndex] = std::move(Data[LastIndex]);
		}
		Data.pop_back();
	}

private:
	VectorType Data;
};

template <typename TKey, typename TValue>
struct TPair
{
	TKey Key;
	TValue Value;
};

// --- Sorted Flat Map (Deterministic iteration order for replays, backed by PMR arena) ---
template <typename TKey, typename TValue, typename TCompare = std::less<TKey>>
class TMap
{
public:
	using FEntry = TPair<TKey, TValue>;
	using mapped_type = TValue;
	using TAllocatorType = std::pmr::polymorphic_allocator<FEntry>;
	using EntryAllocatorType = typename std::allocator_traits<TAllocatorType>::template rebind_alloc<FEntry>;
	using VectorType = TArray<FEntry, EntryAllocatorType>;
	using iterator = typename VectorType::iterator;
	using const_iterator = typename VectorType::const_iterator;
	using Iterator = iterator;
	using ConstIterator = const_iterator;

	TMap() : Entries(std::pmr::get_default_resource()) {}

	explicit TMap(std::pmr::memory_resource* InResource)
		: Entries(InResource)
	{}

	TMap(std::initializer_list<FEntry> InInit, std::pmr::memory_resource* InResource = std::pmr::get_default_resource())
		: Entries(InResource)
	{
		Entries.Reserve(InInit.size());
		for (const auto& Item : InInit)
		{
			InsertOrAssign(Item.Key, Item.Value);
		}
	}

	template <typename TString = TKey, typename = std::enable_if_t<std::is_constructible_v<TString, std::string_view> && !std::is_convertible_v<std::string_view, TString>>>
	TMap(std::initializer_list<std::pair<std::string_view, TValue>> InInit, std::pmr::memory_resource* InResource = std::pmr::get_default_resource())
		: Entries(InResource)
	{
		Entries.Reserve(InInit.size());
		for (const auto& Item : InInit)
		{
			InsertOrAssign(TKey(Item.first), Item.second);
		}
	}
	friend bool operator==(const TMap& InLeft, const TMap& InRight)
	{
		if (InLeft.Size() != InRight.Size())
		{
			return false;
		}
		for (SizeType Index = 0; Index < InLeft.Size(); ++Index)
		{
			const FEntry& Left = InLeft.Entries[Index];
			const FEntry& Right = InRight.Entries[Index];
			if (!(Left.Key == Right.Key) || !(Left.Value == Right.Value))
			{
				return false;
			}
		}
		return true;
	}

	friend bool operator!=(const TMap& InLeft, const TMap& InRight)
	{
		return !(InLeft == InRight);
	}
	TValue& operator[](const TKey& InKey)
	{
		auto It = LowerBound(InKey);
		if (It != Entries.end() && !Comp(InKey, It->Key))
		{
			return It->Value;
		}
		It = Entries.Insert(It, FEntry{ InKey, TValue() });
		return It->Value;
	}

	template <typename TString = TKey, typename = std::enable_if_t<std::is_constructible_v<TString, std::string_view> && !std::is_convertible_v<std::string_view, TString>>>
	TValue& operator[](const std::string_view InKey)
	{
		return (*this)[TKey(InKey)];
	}

	template <typename TVal, typename TString = TKey, typename = std::enable_if_t<std::is_constructible_v<TString, std::string_view> && !std::is_convertible_v<std::string_view, TString>>>
	TValue& InsertOrAssign(const std::string_view InKey, TVal&& InValue)
	{
		return InsertOrAssign(TKey(InKey), std::forward<TVal>(InValue));
	}

	template <typename TVal>
	TValue& InsertOrAssign(const TKey& InKey, TVal&& InValue)
	{
		auto It = LowerBound(InKey);
		if (It != Entries.end() && !Comp(InKey, It->Key))
		{
			It->Value = std::forward<TVal>(InValue);
			return It->Value;
		}
		It = Entries.Insert(It, FEntry{ InKey, std::forward<TVal>(InValue) });
		return It->Value;
	}

	TValue* Find(const std::string_view InKey)
	{
		if constexpr (std::is_same_v<TKey, FPersistentString>)
		{
			const FPersistentString Key = FPersistentString::Find(InKey);
			return Key.IsValid() ? Find(Key) : nullptr;
		}
		else if constexpr (std::is_same_v<TKey, FTransientString>)
		{
			const FTransientString Key = FTransientString::Find(InKey);
			return Key.IsValid() ? Find(Key) : nullptr;
		}
		else if constexpr (std::is_constructible_v<TKey, std::string_view>)
		{
			return Find(TKey(InKey));
		}
		else
		{
			static_assert(std::is_constructible_v<TKey, std::string_view> || std::is_convertible_v<std::string_view, TKey>,
				"TKey cannot be constructed or converted from std::string_view");
			return nullptr;
		}
	}

	const TValue* Find(const std::string_view InKey) const
	{
		return const_cast<TMap*>(this)->Find(InKey);
	}

	TValue* Find(const TKey& InKey) noexcept
	{
		auto It = LowerBound(InKey);
		return (It != Entries.end() && !Comp(InKey, It->Key)) ? &It->Value : nullptr;
	}

	const TValue* Find(const TKey& InKey) const noexcept
	{
		auto It = LowerBound(InKey);
		return (It != Entries.end() && !Comp(InKey, It->Key)) ? &It->Value : nullptr;
	}

	bool Contains(const TKey& InKey) const noexcept
	{
		return Find(InKey) != nullptr;
	}

	bool Remove(const TKey& InKey)
	{
		auto It = LowerBound(InKey);
		if (It != Entries.end() && !Comp(InKey, It->Key))
		{
			Entries.Erase(It);
			return true;
		}
		return false;
	}

	iterator Erase(const_iterator InPos) { return Entries.Erase(InPos); }

	void Reserve(const SizeType InCapacity) { Entries.Reserve(InCapacity); }
	void Clear() noexcept { Entries.Clear(); }

	SizeType Size() const noexcept { return Entries.Size(); }
	bool IsEmpty() const noexcept { return Entries.IsEmpty(); }

	iterator begin() noexcept { return Entries.begin(); }
	iterator end() noexcept { return Entries.end(); }
	const_iterator begin() const noexcept { return Entries.begin(); }
	const_iterator end() const noexcept { return Entries.end(); }

private:
	iterator LowerBound(const TKey& InKey) noexcept
	{
		return std::lower_bound(Entries.begin(), Entries.end(), InKey,
			[this](const FEntry& InEntry, const TKey& InK) { return Comp(InEntry.Key, InK); });
	}

	const_iterator LowerBound(const TKey& InKey) const noexcept
	{
		return std::lower_bound(Entries.begin(), Entries.end(), InKey,
			[this](const FEntry& InEntry, const TKey& InK) { return Comp(InEntry.Key, InK); });
	}

	VectorType Entries;
	[[msvc::no_unique_address]] TCompare Comp{};
};

// --- Sorted Flat Set (Deterministic iteration order, backed by PMR arena) ---
template <typename TKey, typename TCompare = std::less<TKey>>
class TSet
{
public:
	using VectorType = TArray<TKey>;
	using iterator = typename VectorType::iterator;
	using const_iterator = typename VectorType::const_iterator;

	TSet() : Keys(std::pmr::get_default_resource()) {}

	explicit TSet(std::pmr::memory_resource* InResource)
		: Keys(std::pmr::polymorphic_allocator<TKey>(InResource))
	{}

	TSet(std::initializer_list<TKey> InInit)
		: Keys(std::pmr::get_default_resource())
	{
		Keys.Reserve(InInit.size());
		for (const TKey& Key : InInit)
		{
			Insert(Key);
		}
	}

	template <typename TString = TKey, typename = std::enable_if_t<std::is_constructible_v<TString, std::string_view> && !std::is_convertible_v<std::string_view, TString>>>
	TSet(std::initializer_list<std::string_view> InInit)
		: Keys(std::pmr::get_default_resource())
	{
		Keys.Reserve(InInit.size());
		for (const std::string_view Key : InInit)
		{
			Insert(TKey(Key));
		}
	}

	friend bool operator==(const TSet& InLeft, const TSet& InRight)
	{
		if (InLeft.Size() != InRight.Size())
		{
			return false;
		}
		for (SizeType Index = 0; Index < InLeft.Size(); ++Index)
		{
			if (!(InLeft.Keys[Index] == InRight.Keys[Index]))
			{
				return false;
			}
		}
		return true;
	}

	friend bool operator!=(const TSet& InLeft, const TSet& InRight)
	{
		return !(InLeft == InRight);
	}

	bool Insert(const TKey& InKey)
	{
		auto It = LowerBound(InKey);
		if (It != Keys.end() && !Comp(InKey, *It))
		{
			return false;
		}
		Keys.Insert(It, InKey);
		return true;
	}

	template <typename TString = TKey, typename = std::enable_if_t<std::is_constructible_v<TString, std::string_view> && !std::is_convertible_v<std::string_view, TString>>>
	bool Insert(const std::string_view InKey)
	{
		return Insert(TKey(InKey));
	}

	bool Contains(const TKey& InKey) const noexcept
	{
		auto It = LowerBound(InKey);
		return It != Keys.end() && !Comp(InKey, *It);
	}

	bool Contains(const std::string_view InKey) const
	{
		if constexpr (std::is_same_v<TKey, FPersistentString> || std::is_same_v<TKey, FTransientString>)
		{
			const TKey Key = TKey::Find(InKey);
			return Key.IsValid() && Contains(Key);
		}
		else
		{
			static_assert(std::is_constructible_v<TKey, std::string_view>, "TKey cannot be constructed from std::string_view");
			return Contains(TKey(InKey));
		}
	}

	bool Remove(const TKey& InKey)
	{
		auto It = LowerBound(InKey);
		if (It == Keys.end() || Comp(InKey, *It))
		{
			return false;
		}
		Keys.Erase(It);
		return true;
	}

	void Reserve(const SizeType InCapacity) { Keys.Reserve(InCapacity); }
	void Clear() noexcept { Keys.Clear(); }

	SizeType Size() const noexcept { return Keys.Size(); }
	bool IsEmpty() const noexcept { return Keys.IsEmpty(); }

	iterator begin() noexcept { return Keys.begin(); }
	iterator end() noexcept { return Keys.end(); }
	const_iterator begin() const noexcept { return Keys.begin(); }
	const_iterator end() const noexcept { return Keys.end(); }

private:
	iterator LowerBound(const TKey& InKey) noexcept
	{
		return std::lower_bound(Keys.begin(), Keys.end(), InKey, Comp);
	}

	const_iterator LowerBound(const TKey& InKey) const noexcept
	{
		return std::lower_bound(Keys.begin(), Keys.end(), InKey, Comp);
	}

	VectorType Keys;
	[[msvc::no_unique_address]] TCompare Comp{};
};
// --- Small Array (Inline storage spilling to PMR Arena) ---
template <typename T, SizeType InlineCapacity>
class TSmallArray
{
public:
	explicit TSmallArray(std::pmr::memory_resource* InResource = std::pmr::get_default_resource()) noexcept
		: SpilledData(InResource)
	{}

	~TSmallArray()
	{
		if (!bSpilled)
		{
			DestroyInline();
		}
	}

	TSmallArray(const TSmallArray& InOther)
		: SpilledData(InOther.SpilledData.GetAllocator())
	{
		Reserve(InOther.Size());
		for (const T& Item : InOther)
		{
			PushBack(Item);
		}
	}

	TSmallArray& operator=(const TSmallArray& InOther)
	{
		if (this != &InOther)
		{
			Clear();
			Reserve(InOther.Size());
			for (const T& Item : InOther)
			{
				PushBack(Item);
			}
		}
		return *this;
	}

	TSmallArray(TSmallArray&& InOther) noexcept
		: SpilledData(InOther.SpilledData.GetAllocator())
	{
		MoveFrom(std::move(InOther));
	}

	TSmallArray& operator=(TSmallArray&& InOther) noexcept
	{
		if (this != &InOther)
		{
			if (!bSpilled)
			{
				DestroyInline();
			}
			else
			{
				SpilledData.Clear();
			}
			bSpilled = false;
			MoveFrom(std::move(InOther));
		}
		return *this;
	}

	template <typename... TArgs>
	T& EmplaceBack(TArgs&&... InArgs)
	{
		if (!bSpilled)
		{
			if (InlineCount < InlineCapacity)
			{
				T* Slot = ::new (InlinePtr(InlineCount)) T(std::forward<TArgs>(InArgs)...);
				++InlineCount;
				return *Slot;
			}
			SpillToArena(InlineCapacity * 2 > 0 ? InlineCapacity * 2 : 4);
		}
		return SpilledData.EmplaceBack(std::forward<TArgs>(InArgs)...);
	}

	void PushBack(const T& InVal) { EmplaceBack(InVal); }
	void PushBack(T&& InVal) { EmplaceBack(std::move(InVal)); }

	void Reserve(const SizeType InCapacity)
	{
		if (!bSpilled && InCapacity > InlineCapacity)
		{
			SpillToArena(InCapacity);
		}
		else if (bSpilled)
		{
			SpilledData.Reserve(InCapacity);
		}
	}

	void Clear() noexcept
	{
		if (bSpilled)
		{
			SpilledData.Clear();
		}
		else
		{
			DestroyInline();
		}
	}

	T* Data() noexcept { return bSpilled ? SpilledData.GetData() : InlinePtr(0); }
	const T* Data() const noexcept { return bSpilled ? SpilledData.GetData() : InlinePtr(0); }

	SizeType Size() const noexcept { return bSpilled ? SpilledData.Size() : InlineCount; }
	bool IsEmpty() const noexcept { return Size() == 0; }
	bool IsInline() const noexcept { return !bSpilled; }

	T& operator[](const SizeType InIdx) noexcept { return Data()[InIdx]; }
	const T& operator[](const SizeType InIdx) const noexcept { return Data()[InIdx]; }

	T* begin() noexcept { return Data(); }
	T* end() noexcept { return Data() + Size(); }
	const T* begin() const noexcept { return Data(); }
	const T* end() const noexcept { return Data() + Size(); }

private:
	struct alignas(T) FAlignedSlot
	{
		unsigned char Bytes[sizeof(T)];
	};

	T* InlinePtr(const SizeType InIdx) noexcept
	{
		return std::launder(reinterpret_cast<T*>(&InlineStorage[InIdx]));
	}

	const T* InlinePtr(const SizeType InIdx) const noexcept
	{
		return std::launder(reinterpret_cast<const T*>(&InlineStorage[InIdx]));
	}

	void DestroyInline() noexcept
	{
		for (SizeType i = 0; i < InlineCount; ++i)
		{
			InlinePtr(i)->~T();
		}
		InlineCount = 0;
	}

	void SpillToArena(const SizeType InNewCapacity)
	{
		SpilledData.Reserve(InNewCapacity);
		for (SizeType i = 0; i < InlineCount; ++i)
		{
			SpilledData.EmplaceBack(std::move(*InlinePtr(i)));
		}
		DestroyInline();
		bSpilled = true;
	}

	void MoveFrom(TSmallArray&& InOther) noexcept
	{
		if (InOther.bSpilled)
		{
			// Note: if allocators differ, std::pmr::vector moves elements into this arena
			SpilledData = std::move(InOther.SpilledData);
			bSpilled = true;
			InOther.bSpilled = false;
		}
		else
		{
			for (SizeType i = 0; i < InOther.InlineCount; ++i)
			{
				::new (InlinePtr(i)) T(std::move(*InOther.InlinePtr(i)));
			}
			InlineCount = InOther.InlineCount;
			InOther.DestroyInline();
		}
	}

	FAlignedSlot InlineStorage[InlineCapacity > 0 ? InlineCapacity : 1];
	TArray<T> SpilledData;
	SizeType InlineCount = 0;
	bool bSpilled = false;
};
