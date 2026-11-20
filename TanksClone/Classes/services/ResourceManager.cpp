#include "services/ResourceManager.h"

#include "cocos2d.h"

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"

namespace tanks
{

void ResourceManager::EnsureLoaded()
{
	if ( bLoaded )
	{
		return;
	}
	bLoaded = true;
	if ( !Table.FromConfig(GameContext::Get().GetManager<ConfigReader>().GetConfig(ResourcesConfig)) )
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

std::string ResourceManager::ResolvePath(const std::string_view InId, const model::ResourceKind InKind)
{
	EnsureLoaded();
	const model::ResourceEntry* const Entry = Table.Find(InId);
	if ( !Entry || Entry->Kind != InKind )
	{
		CCLOGERROR("ResourceManager: no %s resource with id %s", model::ResourceTable::GetSectionName(InKind), std::string(InId).c_str());
		return std::string();
	}
	return Entry->Path;
}

} // namespace tanks
