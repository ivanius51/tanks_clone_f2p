#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

/**
 * bPersistent = true  : Lives for the entire lifetime of the application.
 * bPersistent = false : Stored in a transient pool that can be flushed via ClearTransientStrings().
 * A string kept once in a global pool: the handle is a 4-byte id, so copying, comparing and
 * hashing are integer operations, and View() gives the text. Id 0 is the empty string. Names are never freed (config
 * keys, ids and event names live for the whole run). Intern adds a string; Find only looks it up, so a query with an
 * unknown text never grows the pool. ClearTransientStrings invalidates every transient handle still held: clear the
 * owner of the transient strings (e.g. GameEvents) first.
 */
template <bool bPersistent = true>
class TStoredString
{
	friend class TStoredString<!bPersistent>;

public:
	constexpr TStoredString() = default;

	explicit TStoredString(const std::string_view InText)
		: Id(GetStringPool<bPersistent>().Intern(InText))
	{
	}

	static constexpr TStoredString Empty() noexcept { return TStoredString{}; }
	static constexpr TStoredString None() noexcept  { return TStoredString{}; }

	static void ClearTransientStrings()
	{
		GetStringPool<false>().Clear();
	}

	static TStoredString Find(const std::string_view InText)
	{
		TStoredString Result;
		Result.Id = GetStringPool<bPersistent>().Find(InText);
		return Result;
	}

	template <bool bOtherPersistent>
	explicit TStoredString(const TStoredString<bOtherPersistent> InOther)
		: TStoredString(InOther.View())
	{
	}

	std::string_view View() const { return GetStringPool<bPersistent>().View(Id); }
	constexpr std::uint32_t GetId() const noexcept { return Id; }

	constexpr bool IsEmpty() const noexcept { return Id == 0; }
	constexpr bool IsValid() const noexcept { return !IsEmpty(); }
	constexpr explicit operator bool() const noexcept { return IsValid(); }

	friend constexpr bool operator<(const TStoredString InA, const TStoredString InB) noexcept  { return InA.Id < InB.Id; }
	friend bool operator==(const TStoredString InA, const std::string_view InText) { return InA.View() == InText; }
	friend bool operator!=(const TStoredString InA, const std::string_view InText) { return InA.View() != InText; }
	friend bool operator<(const TStoredString InA, const std::string_view InText)  { return InA.View() < InText; }

private:
	class StringPool
	{
	public:
		StringPool()
		{
			Reset();
		}

		std::uint32_t Intern(const std::string_view InText)
		{
			if (InText.empty())
			{
				return 0;
			}

			const std::lock_guard<std::mutex> Lock(Guard);
			const auto Found = Ids.find(InText);
			if (Found != Ids.end())
			{
				return Found->second;
			}

			Texts.emplace_back(InText);
			const std::uint32_t NewId = static_cast<std::uint32_t>(Texts.size() - 1);
			Ids.emplace(std::string_view(Texts.back()), NewId);
			return NewId;
		}

		std::uint32_t Find(const std::string_view InText) const
		{
			if (InText.empty())
			{
				return 0;
			}

			const std::lock_guard<std::mutex> Lock(Guard);
			const auto Found = Ids.find(InText);
			return Found != Ids.end() ? Found->second : 0;
		}

		std::string_view View(const std::uint32_t InId) const
		{
			if (InId == 0)
			{
				return {};
			}

			const std::lock_guard<std::mutex> Lock(Guard);
			return InId < Texts.size() ? std::string_view(Texts[InId]) : std::string_view{};
		}

		void Clear()
		{
			const std::lock_guard<std::mutex> Lock(Guard);
			Reset();
		}

	private:
		void Reset()
		{
			Ids.clear();
			Texts.clear();
			Texts.emplace_back();
			Ids.emplace(std::string_view(Texts.back()), 0);
		}

		mutable std::mutex Guard;
		std::deque<std::string> Texts;
		std::unordered_map<std::string_view, std::uint32_t> Ids;
	};

	template <bool bPoolPersistent>
	static StringPool& GetStringPool()
	{
		return TStoredString<bPoolPersistent>::Pool;
	}

private:
	static inline StringPool Pool;
	std::uint32_t Id = 0;
};

template <bool bPersistentA, bool bPersistentB>
bool operator==(const TStoredString<bPersistentA> InA, const TStoredString<bPersistentB> InB)
{
	if constexpr (bPersistentA == bPersistentB)
	{
		return InA.GetId() == InB.GetId();
	}
	else
	{
		return InA.View() == InB.View();
	}
}

template <bool bPersistentA, bool bPersistentB>
bool operator!=(const TStoredString<bPersistentA> InA, const TStoredString<bPersistentB> InB)
{
	return !(InA == InB);
}

using FPersistentString = TStoredString<true>;
using FTransientString  = TStoredString<false>;

inline constexpr FPersistentString EmptyPersistentString = FPersistentString::Empty();
inline constexpr FTransientString  EmptyTransientString  = FTransientString::Empty();

template <bool bPersistent>
struct std::hash<TStoredString<bPersistent>>
{
	std::size_t operator()(const TStoredString<bPersistent> InName) const noexcept
	{
		return InName.GetId();
	}
};