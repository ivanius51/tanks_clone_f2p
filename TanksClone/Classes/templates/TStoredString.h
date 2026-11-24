#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace tanks
{

/**
 * A string kept once in a global pool (like Unreal's FName): the handle is a 4-byte id, so copying, comparing and
 * hashing are integer operations, and View() gives the text. Id 0 is the empty string. Names are never freed (config
 * keys, ids and event names live for the whole run). Intern adds a string; Find only looks it up, so a query with an
 * unknown text never grows the pool.
 */
class TStoredString
{
public:
	TStoredString() = default;

	explicit TStoredString(const std::string_view InText)
		: Id(Pool().Intern(InText))
	{
	}

	static TStoredString Find(const std::string_view InText)
	{
		TStoredString Result;
		Result.Id = Pool().Find(InText);
		return Result;
	}

	std::string_view View() const { return Pool().View(Id); }
	std::uint32_t GetId() const { return Id; }
	bool IsEmpty() const { return Id == 0; }

	friend bool operator==(const TStoredString InA, const TStoredString InB) { return InA.Id == InB.Id; }
	friend bool operator!=(const TStoredString InA, const TStoredString InB) { return InA.Id != InB.Id; }
	friend bool operator<(const TStoredString InA, const TStoredString InB) { return InA.Id < InB.Id; }
	friend bool operator==(const TStoredString InA, const std::string_view InText) { return InA.View() == InText; }

private:
	class StringPool
	{
	public:
		StringPool() { Texts.emplace_back(); }

		std::uint32_t Intern(const std::string_view InText)
		{
			const std::lock_guard<std::mutex> Lock(Guard);
			const auto Found = Ids.find(InText);
			if ( Found != Ids.end() )
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
			const std::lock_guard<std::mutex> Lock(Guard);
			const auto Found = Ids.find(InText);
			return Found != Ids.end() ? Found->second : 0;
		}

		std::string_view View(const std::uint32_t InId) const
		{
			const std::lock_guard<std::mutex> Lock(Guard);
			return Texts[InId];
		}

	private:
		mutable std::mutex Guard;
		std::deque<std::string> Texts;
		std::unordered_map<std::string_view, std::uint32_t> Ids;
	};

	static StringPool& Pool()
	{
		static StringPool Instance;
		return Instance;
	}

	std::uint32_t Id = 0;
};

} // namespace tanks

template <>
struct std::hash<tanks::TStoredString>
{
	std::size_t operator()(const tanks::TStoredString InName) const noexcept { return InName.GetId(); }
};
