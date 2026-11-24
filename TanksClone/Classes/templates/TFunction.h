#pragma once

#include <cassert>
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>

#include "templates/TFunction.h"

namespace tanks
{

constexpr std::size_t DefaultFunctionInlineSize = 128;

template <typename TSignature, std::size_t InlineSize = DefaultFunctionInlineSize>
class TFunction;

/**
 * An owning, copyable callable of the signature TResult(TArgs...). The captured state is stored inside the object
 * (InlineSize bytes), never on the heap: a callable that does not fit fails to compile, so the size is chosen on purpose.
 * Replaces TFunction for stored handlers; use TFunctionRef for a callback that is only called during the call.
 */
template <typename TResult, typename... TArgs, std::size_t InlineSize>
class TFunction<TResult(TArgs...), InlineSize>
{
public:
	TFunction() = default;
	TFunction(std::nullptr_t) {}

	template <typename TCallable,
		typename = std::enable_if_t<!std::is_same_v<std::decay_t<TCallable>, TFunction> && std::is_invocable_r_v<TResult, std::decay_t<TCallable>&, TArgs...>>>
	TFunction(TCallable&& InCallable)
	{
		Emplace(std::forward<TCallable>(InCallable));
	}

	TFunction(const TFunction& InOther)
	{
		CopyFrom(InOther);
	}

	TFunction(TFunction&& InOther) noexcept
	{
		MoveFrom(InOther);
	}

	TFunction& operator=(const TFunction& InOther)
	{
		if ( this != &InOther )
		{
			Reset();
			CopyFrom(InOther);
		}
		return *this;
	}

	TFunction& operator=(TFunction&& InOther) noexcept
	{
		if ( this != &InOther )
		{
			Reset();
			MoveFrom(InOther);
		}
		return *this;
	}

	TFunction& operator=(std::nullptr_t)
	{
		Reset();
		return *this;
	}

	template <typename TCallable,
		typename = std::enable_if_t<!std::is_same_v<std::decay_t<TCallable>, TFunction> && std::is_invocable_r_v<TResult, std::decay_t<TCallable>&, TArgs...>>>
	TFunction& operator=(TCallable&& InCallable)
	{
		Reset();
		Emplace(std::forward<TCallable>(InCallable));
		return *this;
	}

	~TFunction()
	{
		Reset();
	}

	TResult operator()(TArgs... InArgs) const
	{
		assert(Table && "calling an empty TFunction");
		return Table->Invoke(Storage, std::forward<TArgs>(InArgs)...);
	}

	explicit operator bool() const { return Table != nullptr; }
	bool IsBound() const { return Table != nullptr; }

	void Reset()
	{
		if ( Table )
		{
			Table->Destroy(Storage);
			Table = nullptr;
		}
	}

	friend bool operator==(const TFunction& InFunction, std::nullptr_t) { return !InFunction.Table; }
	friend bool operator!=(const TFunction& InFunction, std::nullptr_t) { return InFunction.Table != nullptr; }

private:
	struct Buffer
	{
		alignas(std::max_align_t) unsigned char Bytes[InlineSize];
	};

	struct Operations
	{
		TResult (*Invoke)(const Buffer&, TArgs&&...);
		void (*Copy)(Buffer&, const Buffer&);
		void (*Move)(Buffer&, Buffer&);
		void (*Destroy)(Buffer&);
	};

	template <typename TStored>
	static TStored& Access(const Buffer& InBuffer)
	{
		return *std::launder(reinterpret_cast<TStored*>(const_cast<unsigned char*>(InBuffer.Bytes)));
	}

	template <typename TStored>
	static void (*GetCopyOperation())(Buffer&, const Buffer&)
	{
		if constexpr ( std::is_copy_constructible_v<TStored> )
		{
			return [](Buffer& OutTo, const Buffer& InFrom) { new (OutTo.Bytes) TStored(Access<TStored>(InFrom)); };
		}
		else
		{
			return nullptr;
		}
	}

	template <typename TStored>
	static const Operations* GetOperations()
	{
		static const Operations Table = {
			[](const Buffer& InBuffer, TArgs&&... InArgs) -> TResult { return Access<TStored>(InBuffer)(std::forward<TArgs>(InArgs)...); },
			GetCopyOperation<TStored>(),
			[](Buffer& OutTo, Buffer& InFrom) { new (OutTo.Bytes) TStored(std::move(Access<TStored>(InFrom))); },
			[](Buffer& InBuffer) { Access<TStored>(InBuffer).~TStored(); }
		};
		return &Table;
	}

	template <typename TCallable>
	void Emplace(TCallable&& InCallable)
	{
		using TStored = std::decay_t<TCallable>;
		static_assert(sizeof(TStored) <= InlineSize, "the callable's capture is bigger than InlineSize: capture less or raise InlineSize");
		static_assert(alignof(TStored) <= alignof(std::max_align_t), "over-aligned callable");
		new (Storage.Bytes) TStored(std::forward<TCallable>(InCallable));
		Table = GetOperations<TStored>();
	}

	void CopyFrom(const TFunction& InOther)
	{
		if ( InOther.Table )
		{
			assert(InOther.Table->Copy && "copying a TFunction that holds a move-only callable");
			InOther.Table->Copy(Storage, InOther.Storage);
			Table = InOther.Table;
		}
	}

	void MoveFrom(TFunction& InOther)
	{
		if ( InOther.Table )
		{
			InOther.Table->Move(Storage, InOther.Storage);
			Table = InOther.Table;
			InOther.Reset();
		}
	}

	mutable Buffer Storage;
	const Operations* Table = nullptr;
};

} // namespace tanks
