#pragma once

#include <atomic>
#include <cstddef>
#include <utility>

namespace tanks::model
{
/**
 * Base of objects shared through Handle<T>. The count is atomic, so handles may be released from
 * a loading thread. The last handle deletes the object; its destructor frees what it holds.
 */
class RefCounted
{
public:
	RefCounted(const RefCounted&) = delete;
	RefCounted& operator=(const RefCounted&) = delete;

	void AddRef() const noexcept
	{
		Count.fetch_add(1, std::memory_order_relaxed);
	}

	void Release() const noexcept
	{
		if ( Count.fetch_sub(1, std::memory_order_acq_rel) == 1 )
		{
			delete this;
		}
	}

	int GetRefCount() const noexcept
	{
		return Count.load(std::memory_order_relaxed);
	}

protected:
	RefCounted() = default;
	virtual ~RefCounted() = default;

private:
	mutable std::atomic<int> Count{ 0 };
};

/** Owning pointer to a RefCounted object: keeps it alive while any handle exists. */
template <typename TResource>
class Handle
{
public:
	Handle() = default;

	explicit Handle(TResource* const InResource) : Resource(InResource)
	{
		if ( Resource )
		{
			Resource->AddRef();
		}
	}

	Handle(const Handle& InOther) : Handle(InOther.Resource)
	{
	}

	Handle(Handle&& InOther) noexcept : Resource(std::exchange(InOther.Resource, nullptr))
	{
	}

	Handle& operator=(Handle InOther) noexcept
	{
		std::swap(Resource, InOther.Resource);
		return *this;
	}

	~Handle()
	{
		if ( Resource )
		{
			Resource->Release();
		}
	}

	TResource* Get() const
	{
		return Resource;
	}

	TResource* operator->() const
	{
		return Resource;
	}

	TResource& operator*() const
	{
		return *Resource;
	}

	explicit operator bool() const
	{
		return Resource != nullptr;
	}

	void Reset()
	{
		*this = Handle();
	}

private:
	TResource* Resource = nullptr;
};
} // namespace tanks::model
