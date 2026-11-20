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
