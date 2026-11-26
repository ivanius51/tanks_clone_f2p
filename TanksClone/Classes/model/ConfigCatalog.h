#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_map>

#include "model/ConfigNode.h"
#include "templates/TContainers.h"

namespace game::model
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
		Items.Clear();
		IndexById.Clear();
	}

	void Add(const TDef& InDef)
	{
		const std::size_t* const Existing = IndexById.Find(std::string_view(InDef.Id));
		if ( Existing )
		{
			Items[*Existing] = InDef;
			return;
		}
		IndexById.InsertOrAssign(FPersistentString(InDef.Id), Items.Size());
		Items.PushBack(InDef);
	}

	void SetAll(const TArray<TDef>& InDefs)
	{
		Clear();
		for ( const TDef& Def : InDefs )
		{
			Add(Def);
		}
	}

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
		const std::size_t* const Found = IndexById.Find(InId);
		return Found ? &Items[*Found] : nullptr;
	}

	bool Contains(std::string_view InId) const
	{
		return Find(InId) != nullptr;
	}

	const TArray<TDef>& GetAll() const
	{
		return Items;
	}

	bool IsEmpty() const
	{
		return Items.IsEmpty();
	}

private:
	TArray<TDef> Items;
	TMap<FPersistentString, std::size_t> IndexById;
};
} // namespace game::model
