#include "model/ResourceTable.h"

namespace game::model
{

const char* ResourceTable::GetSectionName(const ResourceKind InKind)
{
	switch ( InKind )
	{
		case ResourceKind::Sound: return "sounds";
		case ResourceKind::Atlas: return "atlases";
		case ResourceKind::Font: return "fonts";
		case ResourceKind::Config: return "configs";
		case ResourceKind::Count: break;
	}
	return "";
}

bool ResourceTable::Set(const std::string_view InId, const ResourceKind InKind, const std::string_view InPath)
{
	const FPersistentString Id{ InId };
	ResourceEntry* const Found = Entries.Find(Id);
	if ( !Found )
	{
		Entries.InsertOrAssign(Id, ResourceEntry{ InKind, FPersistentString{ InPath } });
		++KindCounts[InKind];
		return true;
	}
	if ( Found->Kind != InKind )
	{
		return false;
	}
	Found->Path = FPersistentString{ InPath };
	return true;
}

bool ResourceTable::ReadSections(const IConfigNode& InNode, const ConfigResolver& InResolver, std::size_t& OutChanged)
{
	bool bOk = true;
	for ( SizeType KindIndex = 0; KindIndex < KindCounts.Size; ++KindIndex )
	{
		const ResourceKind Kind = static_cast<ResourceKind>(KindIndex);
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
	const FPersistentString Key = FPersistentString::Find(InName);
	if ( !Key.IsValid() || !Entries.Contains(Key) )
	{
		return ResourceId();
	}
	return ResourceId(Key.View());
}

TArray<FPersistentString> ResourceTable::GetIds(const ResourceKind InKind) const
{
	TArray<FPersistentString> Result;
	Result.Reserve(KindCounts[InKind]);
	for ( const auto& Current : Entries )
	{
		if ( Current.Value.Kind == InKind )
		{
			Result.EmplaceBack(Current.Key);
		}
	}
	return Result;
}

TArray<FPersistentString> ResourceTable::GetPaths(const ResourceKind InKind) const
{
	TArray<FPersistentString> Result;
	Result.Reserve(KindCounts[InKind]);
	for ( const auto& Current : Entries )
	{
		if ( Current.Value.Kind == InKind )
		{
			Result.EmplaceBack(Current.Value.Path);
		}
	}
	return Result;
}

} // namespace game::model
