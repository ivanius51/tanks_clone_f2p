#pragma once

#include <memory>
#include <type_traits>
#include <utility>

namespace tanks
{

template <typename TSignature>
class TFunctionRef;

/**
 * A non-owning reference to a callable of the signature TResult(TArgs...): two pointers, no allocation.
 * For a callback that is only called while the function runs (for-each, predicates). Never store it:
 * the callable it points to is usually a temporary of the call.
 */
template <typename TResult, typename... TArgs>
class TFunctionRef<TResult(TArgs...)>
{
public:
	template <typename TCallable,
		typename = std::enable_if_t<!std::is_same_v<std::decay_t<TCallable>, TFunctionRef> && std::is_invocable_r_v<TResult, TCallable&, TArgs...>>>
	TFunctionRef(TCallable&& InCallable)
		: Object(const_cast<void*>(static_cast<const void*>(std::addressof(InCallable))))
		, Call([](void* InObject, TArgs... InArgs) -> TResult { return (*static_cast<std::remove_reference_t<TCallable>*>(InObject))(std::forward<TArgs>(InArgs)...); })
	{
	}

	TResult operator()(TArgs... InArgs) const
	{
		return Call(Object, std::forward<TArgs>(InArgs)...);
	}

private:
	void* Object;
	TResult (*Call)(void*, TArgs...);
};

} // namespace tanks
