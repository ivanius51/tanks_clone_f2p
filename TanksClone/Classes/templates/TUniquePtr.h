#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

namespace game
{

/**
 * Sole owner of a heap object: move-only, deletes it when it goes out of scope. A TUniquePtr<TDerived> converts to a
 * TUniquePtr<TBase>; the base needs a virtual destructor. Same job as std::unique_ptr without the deleter parameter.
 */
template <typename T>
class TUniquePtr
{
public:
	constexpr TUniquePtr() noexcept = default;
	constexpr TUniquePtr(std::nullptr_t) noexcept {}
	explicit TUniquePtr(T* const InObject) noexcept : Object(InObject) {}

	TUniquePtr(const TUniquePtr&) = delete;
	TUniquePtr& operator=(const TUniquePtr&) = delete;

	TUniquePtr(TUniquePtr&& InOther) noexcept : Object(InOther.Release()) {}

	template <typename TOther, typename = std::enable_if_t<std::is_convertible_v<TOther*, T*>>>
	TUniquePtr(TUniquePtr<TOther>&& InOther) noexcept : Object(InOther.Release()) {}

	~TUniquePtr() { delete Object; }

	TUniquePtr& operator=(TUniquePtr&& InOther) noexcept
	{
		if (this != &InOther)
		{
			Reset(InOther.Release());
		}
		return *this;
	}

	TUniquePtr& operator=(std::nullptr_t) noexcept
	{
		Reset();
		return *this;
	}

	T* Get() const noexcept { return Object; }
	T* operator->() const noexcept { return Object; }
	T& operator*() const noexcept { return *Object; }
	explicit operator bool() const noexcept { return Object != nullptr; }

	[[nodiscard]] T* Release() noexcept { return std::exchange(Object, nullptr); }

	void Reset(T* const InObject = nullptr) noexcept
	{
		T* const Old = std::exchange(Object, InObject);
		delete Old;
	}

	friend bool operator==(const TUniquePtr& InPtr, std::nullptr_t) noexcept { return InPtr.Object == nullptr; }
	friend bool operator!=(const TUniquePtr& InPtr, std::nullptr_t) noexcept { return InPtr.Object != nullptr; }
	friend bool operator==(std::nullptr_t, const TUniquePtr& InPtr) noexcept { return InPtr.Object == nullptr; }
	friend bool operator!=(std::nullptr_t, const TUniquePtr& InPtr) noexcept { return InPtr.Object != nullptr; }

private:
	T* Object = nullptr;
};

template <typename T, typename... TArgs>
[[nodiscard]] TUniquePtr<T> MakeUnique(TArgs&&... InArgs)
{
	return TUniquePtr<T>(new T(std::forward<TArgs>(InArgs)...));
}

} // namespace game
