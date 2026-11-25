#include "model/ResourceTable.h"

#include <array>

namespace game::model
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
	auto* Found = Entries.Find(InId);
	if ( !Found )
	{
		Entries[InId] = ResourceEntry{ InKind, InPath };
	}
	if (Found->Kind != InKind)
	{
		return false;
	}
	Found->Path = InPath;
	return true;
}

bool ResourceTable::ReadSections(const IConfigNode& InNode, const ConfigResolver& InResolver, std::size_t& OutChanged)
{
	bool bOk = true;
	for ( const ResourceKind Kind : AllKinds )
	{
		const char* const SectionName = GetSectionName(Kind);
		const IConfigNode* Section = InNode.FindChild(SectionName);
		std::string File;
		if ( Section && Section->ReadSelf(File) )
		{
			const IConfigNode* const Loaded = InResolver ? InResolver(File) : nullptr;
			Section = Loaded ? Loaded->FindChild(SectionName) : nullptr;
			bOk = bOk && Section != nullptr;
		}
		if ( !Section )
		{
			continue;
		}
		for ( const std::string& Id : Section->GetKeys() )
		{
			std::string Path;
			if ( Section->TryRead(Id, Path) && Set(Id, Kind, Path) )
			{
				++OutChanged;
			}
			else
			{
				bOk = false;
			}
		}
	}
	return bOk;
}

bool ResourceTable::FromConfig(const IConfigNode& InNode, const ConfigResolver& InResolver)
{
	std::size_t Changed = 0;
	return ReadSections(InNode, InResolver, Changed);
}

std::size_t ResourceTable::ApplyOverrides(const IConfigNode& InNode, const ConfigResolver& InResolver)
{
	std::size_t Changed = 0;
	ReadSections(InNode, InResolver, Changed);
	return Changed;
}

const ResourceEntry* ResourceTable::Find(const std::string_view InId) const
{
	return Entries.Find(InId);
}

const ResourceEntry* ResourceTable::Find(const ResourceId& InId) const
{
	return Find(InId.GetName());
}

ResourceId ResourceTable::FindId(const std::string_view InName) const
{
	const auto Found = Entries.Find(InName);
	return Found ? ResourceId(InName) : ResourceId();
}

std::vector<std::string> ResourceTable::GetIds(const ResourceKind InKind) const
{
	std::vector<std::string> Result;
	for ( const auto& Current : Entries )
	{
		if ( Current.Value.Kind == InKind )
		{
			Result.push_back(Current.Key);
		}
	}
	return Result;
}

std::vector<std::string> ResourceTable::GetPaths(const ResourceKind InKind) const
{
	std::vector<std::string> Result;
	for ( const auto& Current : Entries )
	{
		if ( Current.Value.Kind == InKind )
		{
			Result.push_back(Current.Value.Path);
		}
	}
	return Result;
}

} // namespace game::model
