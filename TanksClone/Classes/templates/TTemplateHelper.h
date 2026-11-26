#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>
#include <array>
#include "templates/TFunction.h"
#include <utility>
#include <vector>


namespace game
{
	//Types aliases
	using SizeType = std::size_t;
	using int32 = std::int32_t;
	using uint32 = std::uint32_t;
	using int64 = std::int64_t;
	using uint64 = std::uint64_t;
	using int8 = std::int8_t;
	using uint8 = std::uint8_t;
	using int16 = std::int16_t;
	using uint16 = std::uint16_t;
	using byte = std::uint8_t;


template <typename...>
inline constexpr bool TAlwaysFalse = false;

template <typename T>
using TRemoveCVRef_t = std::remove_cv_t<std::remove_reference_t<T>>;

// Non-copyable / non-movable base mixins
struct FNonCopyable
{
	FNonCopyable() = default;
	FNonCopyable(const FNonCopyable&) = delete;
	FNonCopyable& operator=(const FNonCopyable&) = delete;
	FNonCopyable(FNonCopyable&&) = default;
	FNonCopyable& operator=(FNonCopyable&&) = default;
};

struct FNonCopyableNonMovable
{
	FNonCopyableNonMovable() = default;
	FNonCopyableNonMovable(const FNonCopyableNonMovable&) = delete;
	FNonCopyableNonMovable& operator=(const FNonCopyableNonMovable&) = delete;
	FNonCopyableNonMovable(FNonCopyableNonMovable&&) = delete;
	FNonCopyableNonMovable& operator=(FNonCopyableNonMovable&&) = delete;
};

// TOptional is a simple optional type that can hold a value of type T or be empty. It provides methods to check if it has a value, get the value, and reset the optional to an empty state.
template <typename T>
class TOptional
{
public:
	TOptional() = default;
	TOptional(const T& InValue) : Value(InValue), bHasValue(true) {}
	TOptional(T&& InValue) : Value(std::move(InValue)), bHasValue(true) {}

	template <typename... TArgs>
	explicit TOptional(TArgs&&... InArgs) : Value(std::forward<TArgs>(InArgs)...), bHasValue(true) {}

	bool Has() const { return bHasValue; }
	const T& Get() const { return Value; }
	T& Get() { return Value; }

	void Reset() { bHasValue = false; }

	void Set(const T& InValue) { Value = InValue; bHasValue = true; }
	void Set(T&& InValue) { Value = std::move(InValue); bHasValue = true; }
	void Set(TOptional<T>&& InOptional) { if (InOptional.Has()) { Value = std::move(InOptional.Get()); bHasValue = true; } else { Reset(); } }
	void Emplace(T&& InValue) { Value = std::move(InValue); bHasValue = true; }
	void Emplace(const T& InValue) { Value = InValue; bHasValue = true; }
	void Emplace(TOptional<T>&& InOptional) { if (InOptional.Has()) { Value = std::move(InOptional.Get()); bHasValue = true; } else { Reset(); } }


	operator bool() const { return bHasValue; }
	operator const T&() const { return Value; }
	operator T&() { return Value; }

private:
	T Value{};
	bool bHasValue = false;
};

// RAII scope exit guard
template <typename TFunc>
class TScopeExit
{
public:
	explicit TScopeExit(TFunc&& InFunc) noexcept
		: Func(std::forward<TFunc>(InFunc))
	{
	}

	~TScopeExit()
	{
		if (bActive)
		{
			Func();
		}
	}

	void Release() noexcept { bActive = false; }

	TScopeExit(const TScopeExit&) = delete;
	TScopeExit& operator=(const TScopeExit&) = delete;
	TScopeExit(TScopeExit&& InOther) noexcept
		: Func(std::move(InOther.Func))
		, bActive(InOther.bActive)
	{
		InOther.Release();
	}

private:
	TFunc Func;
	bool bActive = true;
};

template <typename TFunc>
[[nodiscard]] auto MakeScopeExit(TFunc&& InFunc) noexcept
{
	return TScopeExit<std::decay_t<TFunc>>(std::forward<TFunc>(InFunc));
}

// std::visit lambda overload helper
template <typename... TCallables>
struct TOverloaded : TCallables...
{
	using TCallables::operator()...;
};

template <typename... TCallables>
TOverloaded(TCallables...) -> TOverloaded<TCallables...>;

// Strong typedef wrapper
template <typename TValue, typename TTag>
struct TStrongType
{
	TValue Value{};

	constexpr TStrongType() = default;
	constexpr explicit TStrongType(TValue InValue) noexcept : Value(std::move(InValue)) {}

	constexpr friend bool operator==(TStrongType A, TStrongType B) noexcept { return A.Value == B.Value; }
	constexpr friend bool operator!=(TStrongType A, TStrongType B) noexcept { return A.Value != B.Value; }
	constexpr friend bool operator<(TStrongType A, TStrongType B) noexcept  { return A.Value < B.Value; }

	constexpr bool IsValid() const noexcept { return Value != TValue{}; }
	constexpr explicit operator bool() const noexcept { return IsValid(); }

	constexpr TStrongType& operator++() noexcept
	{
		++Value;
		return *this;
	}

	constexpr TStrongType operator++(int) noexcept
	{
		TStrongType Copy(*this);
		++Value;
		return Copy;
	}
};

// Enum bitmask helpers
template <typename TEnum>
struct TEnableEnumBitmasks : std::false_type {};

template <typename TEnum>
inline constexpr bool TEnableEnumBitmasks_v = TEnableEnumBitmasks<TEnum>::value;

template <typename TEnum, typename = std::enable_if_t<TEnableEnumBitmasks_v<TEnum>>>
constexpr TEnum operator|(TEnum InA, TEnum InB) noexcept
{
	using TUnderlying = std::underlying_type_t<TEnum>;
	return static_cast<TEnum>(static_cast<TUnderlying>(InA) | static_cast<TUnderlying>(InB));
}

template <typename TEnum, typename = std::enable_if_t<TEnableEnumBitmasks_v<TEnum>>>
constexpr TEnum operator&(TEnum InA, TEnum InB) noexcept
{
	using TUnderlying = std::underlying_type_t<TEnum>;
	return static_cast<TEnum>(static_cast<TUnderlying>(InA) & static_cast<TUnderlying>(InB));
}

template <typename TEnum, typename = std::enable_if_t<TEnableEnumBitmasks_v<TEnum>>>
constexpr TEnum operator^(TEnum InA, TEnum InB) noexcept
{
	using TUnderlying = std::underlying_type_t<TEnum>;
	return static_cast<TEnum>(static_cast<TUnderlying>(InA) ^ static_cast<TUnderlying>(InB));
}

template <typename TEnum, typename = std::enable_if_t<TEnableEnumBitmasks_v<TEnum>>>
constexpr TEnum operator~(TEnum InA) noexcept
{
	using TUnderlying = std::underlying_type_t<TEnum>;
	return static_cast<TEnum>(~static_cast<TUnderlying>(InA));
}

template <typename TEnum, typename = std::enable_if_t<TEnableEnumBitmasks_v<TEnum>>>
constexpr TEnum& operator|=(TEnum& InA, TEnum InB) noexcept
{
	return InA = (InA | InB);
}

template <typename TEnum, typename = std::enable_if_t<TEnableEnumBitmasks_v<TEnum>>>
constexpr TEnum& operator&=(TEnum& InA, TEnum InB) noexcept
{
	return InA = (InA & InB);
}

template <typename TEnum, typename = std::enable_if_t<TEnableEnumBitmasks_v<TEnum>>>
constexpr bool EnumHasAnyFlags(TEnum InFlags, TEnum InContains) noexcept
{
	using TUnderlying = std::underlying_type_t<TEnum>;
	return (static_cast<TUnderlying>(InFlags) & static_cast<TUnderlying>(InContains)) != 0;
}

template <typename TEnum, typename = std::enable_if_t<TEnableEnumBitmasks_v<TEnum>>>
constexpr bool EnumHasAllFlags(TEnum InFlags, TEnum InContains) noexcept
{
	using TUnderlying = std::underlying_type_t<TEnum>;
	return (static_cast<TUnderlying>(InFlags) & static_cast<TUnderlying>(InContains)) == static_cast<TUnderlying>(InContains);
}

template <typename TEnum, typename TValue, std::size_t EnumCount = static_cast<std::size_t>(TEnum::Count)>
class TEnumArray
{
	static_assert(std::is_enum_v<TEnum>, "TEnum must be an enum type");
	static_assert(EnumCount > 0, "TEnum::Count must be greater than 0");

public:
	using UnderlyingType = std::underlying_type_t<TEnum>;
	static constexpr std::size_t Size = EnumCount;

	constexpr TEnumArray() = default;

	constexpr explicit TEnumArray(const TValue& InDefaultValue)
	{
		Data.fill(InDefaultValue);
	}

	constexpr TValue& operator[](const TEnum InKey) noexcept
	{
		return Data[static_cast<std::size_t>(InKey)];
	}

	constexpr const TValue& operator[](const TEnum InKey) const noexcept
	{
		return Data[static_cast<std::size_t>(InKey)];
	}

	constexpr void Fill(const TValue& InValue) { Data.fill(InValue); }

	constexpr auto begin() noexcept { return Data.begin(); }
	constexpr auto end() noexcept { return Data.end(); }
	constexpr auto begin() const noexcept { return Data.begin(); }
	constexpr auto end() const noexcept { return Data.end(); }

private:
	std::array<TValue, EnumCount> Data{};
};

template <typename TEnum>
class TEnumFlags
{
	static_assert(std::is_enum_v<TEnum>, "TEnumFlags requires an enum type");

public:
	using Underlying = std::underlying_type_t<TEnum>;

	constexpr TEnumFlags() noexcept = default;
	constexpr TEnumFlags(const TEnum InFlag) noexcept : Mask(static_cast<Underlying>(InFlag)) {}
	constexpr explicit TEnumFlags(const Underlying InRawMask) noexcept : Mask(InRawMask) {}

	constexpr void Set(const TEnum InFlag) noexcept    { Mask |= static_cast<Underlying>(InFlag); }
	constexpr void Clear(const TEnum InFlag) noexcept  { Mask &= ~static_cast<Underlying>(InFlag); }
	constexpr void Toggle(const TEnum InFlag) noexcept { Mask ^= static_cast<Underlying>(InFlag); }
	constexpr void ClearAll() noexcept                 { Mask = 0; }

	constexpr bool Has(const TEnum InFlag) const noexcept
	{
		return (Mask & static_cast<Underlying>(InFlag)) == static_cast<Underlying>(InFlag);
	}

	constexpr bool HasAny(const TEnumFlags InOther) const noexcept
	{
		return (Mask & InOther.Mask) != 0;
	}

	constexpr bool HasAll(const TEnumFlags InOther) const noexcept
	{
		return (Mask & InOther.Mask) == InOther.Mask;
	}

	constexpr bool IsEmpty() const noexcept { return Mask == 0; }
	constexpr explicit operator bool() const noexcept { return Mask != 0; }
	constexpr Underlying GetRaw() const noexcept { return Mask; }

	constexpr TEnumFlags operator|(const TEnumFlags InOther) const noexcept { return TEnumFlags(Mask | InOther.Mask); }
	constexpr TEnumFlags operator&(const TEnumFlags InOther) const noexcept { return TEnumFlags(Mask & InOther.Mask); }
	constexpr TEnumFlags operator^(const TEnumFlags InOther) const noexcept { return TEnumFlags(Mask ^ InOther.Mask); }
	constexpr TEnumFlags& operator|=(const TEnumFlags InOther) noexcept { Mask |= InOther.Mask; return *this; }
	constexpr TEnumFlags& operator&=(const TEnumFlags InOther) noexcept { Mask &= InOther.Mask; return *this; }

	friend constexpr bool operator==(TEnumFlags InA, TEnumFlags InB) noexcept { return InA.Mask == InB.Mask; }
	friend constexpr bool operator!=(TEnumFlags InA, TEnumFlags InB) noexcept { return InA.Mask != InB.Mask; }

private:
	Underlying Mask = 0;
};

template <typename TFunc>
class TScopeGuard
{
public:
	explicit TScopeGuard(TFunc&& InFunc) noexcept
		: Func(std::forward<TFunc>(InFunc))
	{
	}

	~TScopeGuard()
	{
		if (bActive)
		{
			Func();
		}
	}

	void Dismiss() noexcept { bActive = false; }

	TScopeGuard(const TScopeGuard&) = delete;
	TScopeGuard& operator=(const TScopeGuard&) = delete;
	TScopeGuard(TScopeGuard&& InOther) noexcept
		: Func(std::move(InOther.Func))
		, bActive(InOther.bActive)
	{
		InOther.Dismiss();
	}

private:
	TFunc Func;
	bool bActive = true;
};

template <typename TFunc>
[[nodiscard]] auto MakeScopeGuard(TFunc&& InFunc) noexcept
{
	return TScopeGuard<std::decay_t<TFunc>>(std::forward<TFunc>(InFunc));
}

template <typename T>
class TObserved
{
public:
	using CallbackType = TFunction<void(const T& InNewValue)>;
	using ListenerId = std::uint32_t;

	TObserved() = default;
	explicit TObserved(T InInitialValue) : Value(std::move(InInitialValue)) {}

	const T& Get() const noexcept { return Value; }
	operator const T&() const noexcept { return Value; }

	void Set(const T& InNewValue)
	{
		if (!(Value == InNewValue))
		{
			Value = InNewValue;
			Notify();
		}
	}

	void Set(T&& InNewValue)
	{
		if (!(Value == InNewValue))
		{
			Value = std::move(InNewValue);
			Notify();
		}
	}

	TObserved& operator=(const T& InNewValue) { Set(InNewValue); return *this; }
	TObserved& operator=(T&& InNewValue)      { Set(std::move(InNewValue)); return *this; }

	ListenerId Subscribe(CallbackType InCallback, const bool bNotifyImmediately = false)
	{
		const ListenerId Id = NextId++;
		Listeners.push_back({ Id, std::move(InCallback) });
		if (bNotifyImmediately)
		{
			Listeners.back().Callback(Value);
		}
		return Id;
	}

	void Unsubscribe(const ListenerId InId)
	{
		for (std::size_t i = 0; i < Listeners.size(); ++i)
		{
			if (Listeners[i].Id == InId)
			{
				Listeners[i] = std::move(Listeners.back());
				Listeners.pop_back();
				return;
			}
		}
	}

private:
	struct FListener
	{
		ListenerId Id = 0;
		CallbackType Callback;
	};

	void Notify()
	{
		// Iterate by index in case a listener subscribes/unsubscribes during callback
		for (std::size_t i = 0; i < Listeners.size(); ++i)
		{
			if (Listeners[i].Callback)
			{
				Listeners[i].Callback(Value);
			}
		}
	}

	T Value{};
	std::vector<FListener> Listeners;
	ListenerId NextId = 1;
};

} // namespace game

#define ENABLE_ENUM_BITMASKS(EnumType) \
	template <> struct ::game::TEnableEnumBitmasks<EnumType> : std::true_type {}