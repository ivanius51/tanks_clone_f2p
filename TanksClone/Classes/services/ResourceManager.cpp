#include "services/ResourceManager.h"

#include <fstream>
#include <iterator>

#include "cocos2d.h"

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"

namespace tanks
{

namespace
{
	constexpr const char* PumpKey = "RESOURCE_QUEUE_PUMP";

	model::ResourceTable::ConfigResolver Resolver(const ConfigReader& InReader)
	{
		return [&InReader](const std::string& InPath) { return InReader.HasConfig(InPath) ? &InReader.GetConfig(InPath) : nullptr; };
	}
}

ResourceManager::~ResourceManager()
{
	if ( bPumping )
	{
		cocos2d::Director::getInstance()->getScheduler()->unschedule(PumpKey, this);
	}
}

void ResourceManager::EnsurePumping()
{
	if ( bPumping )
	{
		return;
	}
	bPumping = true;
	cocos2d::Director::getInstance()->getScheduler()->schedule([this](float) { Queue.Pump(); }, this, 0.0f, CC_REPEAT_FOREVER, 0.0f, false, PumpKey);
}

model::WorkQueue& ResourceManager::GetQueue()
{
	EnsurePumping();
	return Queue;
}

void ResourceManager::ReadFileAsync(const std::string& InPath, const std::function<void(std::vector<unsigned char>)>& InDone)
{
	const std::string FullPath = cocos2d::FileUtils::getInstance()->fullPathForFilename(InPath);
	GetQueue().Submit<std::vector<unsigned char>>(
		[FullPath]()
		{
			std::ifstream File(FullPath, std::ios::binary);
			return std::vector<unsigned char>(std::istreambuf_iterator<char>(File), std::istreambuf_iterator<char>());
		},
		InDone);
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

void ResourceManager::AcquireAtlasAsync(const std::string_view InName, const AtlasRegistry::Callback& InDone)
{
	const auto LoadedIt = Atlases->Loaded.find(std::string(InName));
	if ( LoadedIt != Atlases->Loaded.end() )
	{
		InDone(AtlasHandle(LoadedIt->second));
		return;
	}
	const model::ResourceId Id = FindId(InName, model::ResourceKind::Atlas);
	if ( !Id.IsValid() )
	{
		InDone(AtlasHandle());
		return;
	}
	AtlasResource::CreateAsync(std::string(InName), ResolvePath(Id, model::ResourceKind::Atlas), Atlases, GetQueue(), InDone);
}

SoundHandle ResourceManager::AcquireSound(const std::string_view InName)
{
	const auto LoadedIt = Sounds->Loaded.find(std::string(InName));
	if ( LoadedIt != Sounds->Loaded.end() )
	{
		return SoundHandle(LoadedIt->second);
	}
	const model::ResourceId Id = FindId(InName, model::ResourceKind::Sound);
	return Id.IsValid() ? SoundResource::Create(std::string(InName), ResolvePath(Id, model::ResourceKind::Sound), Sounds) : SoundHandle();
}

void ResourceManager::AcquireSoundAsync(const std::string_view InName, const SoundRegistry::Callback& InDone)
{
	const auto LoadedIt = Sounds->Loaded.find(std::string(InName));
	if ( LoadedIt != Sounds->Loaded.end() )
	{
		InDone(SoundHandle(LoadedIt->second));
		return;
	}
	const model::ResourceId Id = FindId(InName, model::ResourceKind::Sound);
	if ( !Id.IsValid() )
	{
		InDone(SoundHandle());
		return;
	}
	SoundResource::CreateAsync(std::string(InName), ResolvePath(Id, model::ResourceKind::Sound), Sounds, InDone);
}

bool ResourceManager::IsSoundLoaded(const std::string_view InName) const
{
	return Sounds->Loaded.count(std::string(InName)) > 0;
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
