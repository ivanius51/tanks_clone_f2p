#pragma once

#include <cassert>
#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace game
{

template <typename TSignature>
class TFunctionRef;

template <typename TResult, typename... TArgs>
class TFunctionRef<TResult(TArgs...)>
{
public:
	constexpr TFunctionRef() noexcept = default;
	constexpr TFunctionRef(std::nullptr_t) noexcept {}

	template <typename TCallable,
		typename = std::enable_if_t<
			!std::is_same_v<std::decay_t<TCallable>, TFunctionRef> &&
			std::is_invocable_r_v<TResult, TCallable&, TArgs...>>>
	TFunctionRef(TCallable&& InCallable) noexcept
	{
		using TRaw = std::remove_reference_t<TCallable>;
		if constexpr (std::is_pointer_v<TRaw> || std::is_member_pointer_v<TRaw>)
		{
			if (InCallable == nullptr)
			{
				return;
			}
		}
		else if constexpr (std::is_constructible_v<bool, const TRaw&>)
		{
			if (!static_cast<bool>(InCallable))
			{
				return;
			}
		}

		Object = const_cast<void*>(static_cast<const void*>(std::addressof(InCallable)));
		Call = [](void* InObject, TArgs... InArgs) -> TResult
		{
			if constexpr (std::is_void_v<TResult>)
			{
				std::invoke(*static_cast<TRaw*>(InObject), std::forward<TArgs>(InArgs)...);
			}
			else
			{
				return std::invoke(*static_cast<TRaw*>(InObject), std::forward<TArgs>(InArgs)...);
			}
		};
	}

	TResult operator()(TArgs... InArgs) const
	{
		assert(Call && "Calling an empty TFunctionRef");
		return Call(Object, std::forward<TArgs>(InArgs)...);
	}

	constexpr explicit operator bool() const noexcept { return Call != nullptr; }
	constexpr bool IsBound() const noexcept { return Call != nullptr; }

	friend constexpr bool operator==(const TFunctionRef& InRef, std::nullptr_t) noexcept { return InRef.Call == nullptr; }
	friend constexpr bool operator!=(const TFunctionRef& InRef, std::nullptr_t) noexcept { return InRef.Call != nullptr; }

private:
	void* Object = nullptr;
	TResult (*Call)(void*, TArgs...) = nullptr;
};

} // namespace game