#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <random>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "templates/TContainers.h"

/**
 * Base of objects shared through THandle<T>. The count is atomic, so handles may be released from
 * a loading thread. The last handle deletes the object; its destructor frees what it holds.
 */
class TRefCounted
{
public:
	TRefCounted(const TRefCounted&) = delete;
	TRefCounted& operator=(const TRefCounted&) = delete;

	void AddRef() const noexcept
	{
		Count.fetch_add(1, std::memory_order_relaxed);
	}

	void Release() const noexcept
	{
		if (Count.fetch_sub(1, std::memory_order_acq_rel) == 1)
		{
			delete this;
		}
	}

	int GetRefCount() const noexcept
	{
		return Count.load(std::memory_order_relaxed);
	}

protected:
	TRefCounted() = default;
	virtual ~TRefCounted() = default;

private:
	mutable std::atomic<int> Count{ 0 };
};

/** Owning pointer to a TRefCounted object: keeps it alive while any handle exists. */
template <typename TResource>
class THandle
{
	static_assert(std::is_base_of_v<TRefCounted, TResource>, "THandle requires TResource to inherit from TRefCounted");

public:
	constexpr THandle() noexcept = default;
	constexpr THandle(std::nullptr_t) noexcept {}

	explicit THandle(TResource* const InResource) noexcept : Resource(InResource)
	{
		if (Resource)
		{
			Resource->AddRef();
		}
	}

	THandle(const THandle& InOther) noexcept : THandle(InOther.Resource)
	{
	}

	THandle(THandle&& InOther) noexcept : Resource(std::exchange(InOther.Resource, nullptr))
	{
	}

	template <typename TOther, typename = std::enable_if_t<std::is_convertible_v<TOther*, TResource*>>>
	THandle(const THandle<TOther>& InOther) noexcept : THandle(static_cast<TResource*>(InOther.Get()))
	{
	}

	THandle& operator=(THandle InOther) noexcept
	{
		std::swap(Resource, InOther.Resource);
		return *this;
	}

	~THandle() noexcept
	{
		if (Resource)
		{
			Resource->Release();
		}
	}

	bool operator==(const THandle& InOther) const noexcept { return Resource == InOther.Resource; }
	bool operator!=(const THandle& InOther) const noexcept { return Resource != InOther.Resource; }
	

	TResource* Get() const noexcept
	{
		return Resource;
	}

	TResource* operator->() const noexcept
	{
		return Resource;
	}

	TResource& operator*() const noexcept
	{
		return *Resource;
	}

	explicit operator bool() const noexcept
	{
		return Resource != nullptr;
	}

	bool IsValid() const noexcept 
	{ 
		return Resource != nullptr; 
	}

	void Reset() noexcept
	{
		if (TResource* Old = std::exchange(Resource, nullptr))
		{
			Old->Release();
		}
	}

	friend bool operator==(const THandle& InHandle, std::nullptr_t) noexcept { return InHandle.Resource == nullptr; }
	friend bool operator==(std::nullptr_t, const THandle& InHandle) noexcept { return InHandle.Resource == nullptr; }
	friend bool operator!=(const THandle& InHandle, std::nullptr_t) noexcept { return InHandle.Resource != nullptr; }
	friend bool operator!=(std::nullptr_t, const THandle& InHandle) noexcept { return InHandle.Resource != nullptr; }
	friend bool operator==(const THandle& InA, const THandle& InB) noexcept { return InA.Resource == InB.Resource; }
	friend bool operator!=(const THandle& InA, const THandle& InB) noexcept { return InA.Resource != InB.Resource; }
	friend bool operator==(const THandle& InA, const TResource* InB) noexcept { return InA.Resource == InB; }
	friend bool operator!=(const THandle& InA, const TResource* InB) noexcept { return InA.Resource != InB; }

	
	template <typename TItem, typename... TArgs>
	[[nodiscard]] static THandle<TItem> Create(TArgs&&... InArgs)
	{
		return THandle<TItem>(new TItem(std::forward<TArgs>(InArgs)...));
	}
	
private:
	TResource* Resource = nullptr;
};

template <typename TResource, typename... TArgs>
[[nodiscard]] THandle<TResource> MakeHandle(TArgs&&... InArgs)
{
	return THandle<TResource>(new TResource(std::forward<TArgs>(InArgs)...));
}

/** A value shared by copies of one handle, e.g. captured by several callbacks; it lives until the last copy is gone. */
template <typename TValue>
class TSharedValue final : public TRefCounted
{
public:
	template <typename... TArgs>
	explicit TSharedValue(TArgs&&... InArgs) : Value(std::forward<TArgs>(InArgs)...) {}

	TValue& Get() { return Value; }
	const TValue& Get() const { return Value; }

private:
	TValue Value;
};

template <typename TValue>
using TSharedHandle = THandle<TSharedValue<TValue>>;

template <typename TValue, typename... TArgs>
[[nodiscard]] TSharedHandle<TValue> MakeSharedValue(TArgs&&... InArgs)
{
	return MakeHandle<TSharedValue<TValue>>(std::forward<TArgs>(InArgs)...);
}


// --- ConfigCatalog ---
template <typename TKey, typename TConfig>
class TConfigCatalog
{
public:
	void Register(const TKey& InId, TConfig InConfig)
	{
		const auto Found = IndexByKey.find(InId);
		if (Found != IndexByKey.end())
		{
			Items[Found->second] = std::move(InConfig);
		}
		else
		{
			IndexByKey.emplace(InId, Items.size());
			Items.push_back(std::move(InConfig));
		}
	}

	const TConfig* Find(const TKey& InId) const noexcept
	{
		const auto Found = IndexByKey.find(InId);
		return Found != IndexByKey.end() ? &Items[Found->second] : nullptr;
	}

	bool Contains(const TKey& InId) const noexcept
	{
		return IndexByKey.find(InId) != IndexByKey.end();
	}

	const std::vector<TConfig>& GetAll() const noexcept { return Items; }
	std::size_t Num() const noexcept { return Items.size(); }

	void Clear()
	{
		Items.clear();
		IndexByKey.clear();
	}

private:
	std::vector<TConfig> Items;
	TMap<TKey, std::size_t> IndexByKey;
};

// --- WeightedTable ---
template <typename T>
class TWeightedTable
{
public:
	void Add(T InItem, const float InWeight)
	{
		if (InWeight <= 0.0f)
		{
			return;
		}
		TotalWeight += InWeight;
		Entries.push_back({ std::move(InItem), TotalWeight });
	}

	template <typename TRng>
	const T* Pick(TRng& InRng) const
	{
		if (Entries.empty())
		{
			return nullptr;
		}
		std::uniform_real_distribution<float> Dist(0.0f, TotalWeight);
		const float Roll = Dist(InRng);

		const auto It = std::lower_bound(
			Entries.begin(),
			Entries.end(),
			Roll,
			[](const FEntry& InEntry, const float InRoll) { return InEntry.CumulativeWeight < InRoll; }
		);

		return It != Entries.end() ? &It->Item : &Entries.back().Item;
	}

	void Clear()
	{
		Entries.clear();
		TotalWeight = 0.0f;
	}

	bool IsEmpty() const noexcept { return Entries.empty(); }
	std::size_t Num() const noexcept { return Entries.size(); }

private:
	struct FEntry
	{
		T Item;
		float CumulativeWeight = 0.0f;
	};

	std::vector<FEntry> Entries;
	float TotalWeight = 0.0f;
};