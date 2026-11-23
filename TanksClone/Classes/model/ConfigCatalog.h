#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "model/ConfigNode.h"

namespace tanks::model
{

/**
 * An ordered list of definitions found by id: the shared base of the books and catalogs built from a config list
 * (achievements, difficulty modes, drop tables...). TDef is a data struct with a public std::string Id.
 * A repeated id replaces the earlier entry, so Find always agrees with the list.
 */
template <typename TDef>
class ConfigCatalog
{
public:
	void Clear()
	{
		Items.clear();
		IndexById.clear();
	}

	void Add(const TDef& InDef)
	{
		const auto Existing = IndexById.find(InDef.Id);
		if ( Existing != IndexById.end() )
		{
			Items[Existing->second] = InDef;
			return;
		}
		IndexById.emplace(InDef.Id, Items.size());
		Items.push_back(InDef);
	}

	void SetAll(const std::vector<TDef>& InDefs)
	{
		Clear();
		for ( const TDef& Def : InDefs )
		{
			Add(Def);
		}
	}

	// Replaces the content with the items of InList that InRead accepts: bool(const IConfigNode& Item, TDef& OutDef).
	template <typename TRead>
	void ReadList(const IConfigNode& InList, TRead InRead)
	{
		Clear();
		for ( std::size_t Index = 0; Index < InList.GetItemCount(); ++Index )
		{
			TDef Def;
			if ( InRead(*InList.GetItem(Index), Def) )
			{
				Add(Def);
			}
		}
	}

	const TDef* Find(std::string_view InId) const
	{
		const auto Found = IndexById.find(std::string(InId));
		return Found != IndexById.end() ? &Items[Found->second] : nullptr;
	}

	bool Contains(std::string_view InId) const
	{
		return Find(InId) != nullptr;
	}

	const std::vector<TDef>& GetAll() const
	{
		return Items;
	}

	bool IsEmpty() const
	{
		return Items.empty();
	}

private:
	std::vector<TDef> Items;
	std::unordered_map<std::string, std::size_t> IndexById;
};

} // namespace tanks::model
