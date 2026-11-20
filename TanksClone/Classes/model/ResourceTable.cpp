#include "model/ResourceTable.h"

#include <array>

namespace tanks::model
{

namespace
{
	constexpr std::array<ResourceKind, 4> AllKinds = { ResourceKind::Sound, ResourceKind::Atlas, ResourceKind::Font, ResourceKind::Config };
}

const char* ResourceTable::GetSectionName(const ResourceKind InKind)
{
	switch ( InKind )
	{
		case ResourceKind::Sound: return "sounds";
		case ResourceKind::Atlas: return "atlases";
		case ResourceKind::Font: return "fonts";
		case ResourceKind::Config: return "configs";
	}
	return "";
}

bool ResourceTable::Set(const std::string& InId, const ResourceKind InKind, const std::string& InPath)
{
	const auto Found = Entries.find(InId);
	if ( Found != Entries.end() && Found->second.Kind != InKind )
	{
		return false;
	}
	Entries[InId] = ResourceEntry{ InKind, InPath };
	return true;
}

bool ResourceTable::FromConfig(const IConfigNode& InNode)
{
	bool bOk = true;
	for ( const ResourceKind Kind : AllKinds )
	{
		const IConfigNode* const Section = InNode.FindChild(GetSectionName(Kind));
		if ( !Section )
		{
			continue;
		}
		for ( const std::string& Id : Section->GetKeys() )
		{
			std::string Path;
			if ( !Section->TryRead(Id, Path) || !Set(Id, Kind, Path) )
			{
				bOk = false;
			}
		}
	}
	return bOk;
}

std::size_t ResourceTable::ApplyOverrides(const IConfigNode& InNode)
{
	std::size_t Changed = 0;
	for ( const ResourceKind Kind : AllKinds )
	{
		const IConfigNode* const Section = InNode.FindChild(GetSectionName(Kind));
		if ( !Section )
		{
			continue;
		}
		for ( const std::string& Id : Section->GetKeys() )
		{
			std::string Path;
			if ( Section->TryRead(Id, Path) && Set(Id, Kind, Path) )
			{
				++Changed;
			}
		}
	}
	return Changed;
}

const ResourceEntry* ResourceTable::Find(const std::string_view InId) const
{
	const auto Found = Entries.find(InId);
	return Found != Entries.end() ? &Found->second : nullptr;
}

std::vector<std::string> ResourceTable::GetIds(const ResourceKind InKind) const
{
	std::vector<std::string> Result;
	for ( const auto& Current : Entries )
	{
		if ( Current.second.Kind == InKind )
		{
			Result.push_back(Current.first);
		}
	}
	return Result;
}

std::vector<std::string> ResourceTable::GetPaths(const ResourceKind InKind) const
{
	std::vector<std::string> Result;
	for ( const auto& Current : Entries )
	{
		if ( Current.second.Kind == InKind )
		{
			Result.push_back(Current.second.Path);
		}
	}
	return Result;
}

} // namespace tanks::model
