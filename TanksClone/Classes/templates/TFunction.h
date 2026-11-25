#pragma once

#include <cassert>
#include <cstddef>
#include <functional>
#include <new>
#include <type_traits>
#include <utility>


constexpr std::size_t DefaultFunctionInlineSize = 64;
constexpr std::size_t UiCallbackInlineSize = 128;

template <typename TSignature, std::size_t InlineSize = DefaultFunctionInlineSize>
class TFunction;

template <typename TResult, typename... TArgs, std::size_t InlineSize>
class TFunction<TResult(TArgs...), InlineSize>
{
public:
	TFunction() noexcept = default;
	TFunction(std::nullptr_t) noexcept {}

	template <typename TCallable,
		typename = std::enable_if_t<
			!std::is_same_v<std::decay_t<TCallable>, TFunction> &&
			std::is_invocable_r_v<TResult, std::decay_t<TCallable>&, TArgs...>>>
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
		if (this != &InOther)
		{
			Reset();
			CopyFrom(InOther);
		}
		return *this;
	}

	TFunction& operator=(TFunction&& InOther) noexcept
	{
		if (this != &InOther)
		{
			Reset();
			MoveFrom(InOther);
		}
		return *this;
	}

	TFunction& operator=(std::nullptr_t) noexcept
	{
		Reset();
		return *this;
	}

	template <typename TCallable,
		typename = std::enable_if_t<
			!std::is_same_v<std::decay_t<TCallable>, TFunction> &&
			std::is_invocable_r_v<TResult, std::decay_t<TCallable>&, TArgs...>>>
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
		assert(Table && "Calling an empty TFunction");
		return Table->Invoke(Storage, std::forward<TArgs>(InArgs)...);
	}

	explicit operator bool() const noexcept { return Table != nullptr; }
	bool IsBound() const noexcept { return Table != nullptr; }

	void Reset() noexcept
	{
		if (Table)
		{
			Table->Destroy(Storage);
			Table = nullptr;
		}
	}

	friend bool operator==(const TFunction& InFunction, std::nullptr_t) noexcept { return !InFunction.Table; }
	friend bool operator!=(const TFunction& InFunction, std::nullptr_t) noexcept { return InFunction.Table != nullptr; }

private:
	struct FBuffer
	{
		alignas(std::max_align_t) unsigned char Bytes[InlineSize];
	};

	struct FOperations
	{
		TResult (*Invoke)(const FBuffer&, TArgs&&...);
		void (*Copy)(FBuffer&, const FBuffer&);
		void (*Move)(FBuffer&, FBuffer&) noexcept;
		void (*Destroy)(FBuffer&) noexcept;
	};

	template <typename TStored>
	static TStored& Access(const FBuffer& InBuffer) noexcept
	{
		return *std::launder(reinterpret_cast<TStored*>(const_cast<unsigned char*>(InBuffer.Bytes)));
	}

	template <typename TStored>
	static constexpr bool IsNullCallable(const TStored& InCallable) noexcept
	{
		if constexpr (std::is_pointer_v<TStored> || std::is_member_pointer_v<TStored>)
		{
			return InCallable == nullptr;
		}
		else if constexpr (std::is_constructible_v<bool, const TStored&>)
		{
			// Handles wrapping another TFunction or std::function that might be empty
			return !static_cast<bool>(InCallable);
		}
		else
		{
			return false;
		}
	}

	template <typename TStored>
	static constexpr auto GetCopyOperation() noexcept -> void (*)(FBuffer&, const FBuffer&)
	{
		if constexpr (std::is_copy_constructible_v<TStored>)
		{
			return [](FBuffer& OutTo, const FBuffer& InFrom)
			{
				::new (OutTo.Bytes) TStored(Access<TStored>(InFrom));
			};
		}
		else
		{
			return nullptr;
		}
	}

	template <typename TStored>
	static const FOperations* GetOperations() noexcept
	{
		static const FOperations Table = {
			[](const FBuffer& InBuffer, TArgs&&... InArgs) -> TResult
			{
				if constexpr (std::is_void_v<TResult>)
				{
					std::invoke(Access<TStored>(InBuffer), std::forward<TArgs>(InArgs)...);
				}
				else
				{
					return std::invoke(Access<TStored>(InBuffer), std::forward<TArgs>(InArgs)...);
				}
			},
			GetCopyOperation<TStored>(),
			[](FBuffer& OutTo, FBuffer& InFrom) noexcept
			{
				::new (OutTo.Bytes) TStored(std::move(Access<TStored>(InFrom)));
			},
			[](FBuffer& InBuffer) noexcept
			{
				Access<TStored>(InBuffer).~TStored();
			}
		};
		return &Table;
	}

	template <typename TCallable>
	void Emplace(TCallable&& InCallable)
	{
		using TStored = std::decay_t<TCallable>;
		static_assert(sizeof(TStored) <= InlineSize, "TFunction: callable capture exceeds InlineSize");
		static_assert(alignof(TStored) <= alignof(std::max_align_t), "TFunction: over-aligned callable");

		if (IsNullCallable(InCallable))
		{
			return;
		}

		::new (Storage.Bytes) TStored(std::forward<TCallable>(InCallable));
		Table = GetOperations<TStored>();
	}

	void CopyFrom(const TFunction& InOther)
	{
		if (InOther.Table)
		{
			assert(InOther.Table->Copy && "Copying a TFunction that holds a move-only callable");
			InOther.Table->Copy(Storage, InOther.Storage);
			Table = InOther.Table;
		}
	}

	void MoveFrom(TFunction& InOther) noexcept
	{
		if (InOther.Table)
		{
			InOther.Table->Move(Storage, InOther.Storage);
			Table = InOther.Table;
			InOther.Reset();
		}
	}

	mutable FBuffer Storage;
	const FOperations* Table = nullptr;
};

// Convenient alias for default-sized function
template <typename TSignature>
using TFunc = TFunction<TSignature, DefaultFunctionInlineSize>;
