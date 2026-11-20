#include "services/ResourceManager.h"

#include "cocos2d.h"

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"

namespace tanks
{

namespace
{
	model::ResourceTable::ConfigResolver Resolver(const ConfigReader& InReader)
	{
		return [&InReader](const std::string& InPath) { return InReader.HasConfig(InPath) ? &InReader.GetConfig(InPath) : nullptr; };
	}
}

void ResourceManager::EnsureLoaded()
{
	if ( bLoaded )
	{
		return;
	}
	bLoaded = true;
	const ConfigReader& Reader = GameContext::Get().GetManager<ConfigReader>();
	if ( !Table.FromConfig(Reader.GetConfig(ResourcesConfig), Resolver(Reader)) )
	{
		CCLOGERROR("ResourceManager: %s has a bad or duplicated resource id", ResourcesConfig);
	}
}

const model::ResourceTable& ResourceManager::GetTable()
{
	EnsureLoaded();
	return Table;
}

void ResourceManager::ApplyOverrides(const model::IConfigNode& InNode)
{
	EnsureLoaded();
	Table.ApplyOverrides(InNode);
}

model::ResourceId ResourceManager::FindId(const std::string_view InName, const model::ResourceKind InKind)
{
	if ( InName.empty() )
	{
		return model::ResourceId();
	}
	EnsureLoaded();
	const model::ResourceEntry* const Entry = Table.Find(InName);
	if ( !Entry || Entry->Kind != InKind )
	{
		CCLOGERROR("ResourceManager: no %s resource with id %s", model::ResourceTable::GetSectionName(InKind), std::string(InName).c_str());
		return model::ResourceId();
	}
	return Table.FindId(InName);
}

AtlasHandle ResourceManager::AcquireAtlas(const std::string_view InName)
{
	const auto LoadedIt = Atlases->Loaded.find(std::string(InName));
	if ( LoadedIt != Atlases->Loaded.end() )
	{
		return AtlasHandle(LoadedIt->second);
	}
	const model::ResourceId Id = FindId(InName, model::ResourceKind::Atlas);
	return Id.IsValid() ? AtlasResource::Create(std::string(InName), ResolvePath(Id, model::ResourceKind::Atlas), Atlases) : AtlasHandle();
}

void ResourceManager::KeepAtlasesLoaded(const std::vector<std::string>& InNames)
{
	for ( const std::string& CurrentName : InNames )
	{
		if ( AtlasHandle Handle = AcquireAtlas(CurrentName) )
		{
			KeptAtlases.push_back(std::move(Handle));
		}
	}
}

bool ResourceManager::IsAtlasLoaded(const std::string_view InName) const
{
	return Atlases->Loaded.count(std::string(InName)) > 0;
}

std::string ResourceManager::GetAtlasLoadedPath(const std::string_view InName) const
{
	const auto LoadedIt = Atlases->Loaded.find(std::string(InName));
	return LoadedIt != Atlases->Loaded.end() ? LoadedIt->second->GetLoadedPath() : std::string();
}

void ResourceManager::ReloadAtlases()
{
	// Sprites already made keep their old frames (they hold the textures); screens are rebuilt after a switch.
	for ( const auto& CurrentAtlas : Atlases->Loaded )
	{
		CurrentAtlas.second->Unload();
	}
	for ( const auto& CurrentAtlas : Atlases->Loaded )
	{
		CurrentAtlas.second->Load();
	}
}

void ResourceManager::SetSmoothPixels(const bool bInSmooth)
{
	Atlases->bSmoothPixels = bInSmooth;
	for ( const auto& CurrentAtlas : Atlases->Loaded )
	{
		CurrentAtlas.second->ApplyFiltering();
	}
}

bool ResourceManager::IsSmoothPixels() const
{
	return Atlases->bSmoothPixels;
}

std::string ResourceManager::ResolvePath(const model::ResourceId& InId, const model::ResourceKind InKind)
{
	EnsureLoaded();
	const model::ResourceEntry* const Entry = Table.Find(InId);
	if ( !Entry || Entry->Kind != InKind )
	{
		CCLOGERROR("ResourceManager: no %s resource with id %s", model::ResourceTable::GetSectionName(InKind), std::string(InId.GetName()).c_str());
		return std::string();
	}
	return Entry->Path;
}

} // namespace tanks
