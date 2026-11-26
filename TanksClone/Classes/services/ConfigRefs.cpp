#include "services/ConfigRefs.h"

#include <algorithm>

#include "core/ConfigReader.h"
#include "core/GameContext.h"
#include "services/ResourceManager.h"
#include "templates/TContainers.h"

namespace game::config_refs
{

FPersistentString ResolveId(const model::ResourceId& InId)
{
	return GameContext::Get<ResourceManager>().ResolvePath(InId, model::ResourceKind::Config);
}

const model::IConfigNode& GetConfig(const model::ResourceId& InId)
{
	return GameContext::Get<ConfigReader>().GetConfig(ResolveId(InId));
}

const model::IConfigNode* FindChild(const model::ResourceId& InId, const std::string_view InKey)
{
	return GetConfig(InId).FindChild(InKey);
}

FPersistentString ResolveName(const std::string_view InName)
{
	if ( InName.empty() )
	{
		return FPersistentString();
	}
	ResourceManager& Resources = GameContext::Get<ResourceManager>();
	const model::ResourceId Id = Resources.FindId(InName, model::ResourceKind::Config);
	return Id.IsValid() ? Resources.ResolvePath(Id, model::ResourceKind::Config) : FPersistentString();
}

TArray<GroupEntry> ListGroupEntries(const std::string_view InGroupPrefix)
{
	ResourceManager& Resources = GameContext::Get<ResourceManager>();
	TArray<std::string> Ids;
	for ( const FPersistentString CurrentId : Resources.GetTable().GetIds(model::ResourceKind::Config) )
	{
		if ( CurrentId.View().compare(0, InGroupPrefix.size(), InGroupPrefix) != 0 )
		{
			continue;
		}
		Ids.EmplaceBack(CurrentId.View());
	}
	std::sort(Ids.begin(), Ids.end(), [](const std::string& InLeft, const std::string& InRight)
	{
		return InLeft.size() != InRight.size() ? InLeft.size() < InRight.size() : InLeft < InRight;
	});
	TArray<GroupEntry> Entries;
	for ( std::string& CurrentId : Ids )
	{
		std::string Path{ Resources.ResolvePath(model::ResourceId(CurrentId), model::ResourceKind::Config).View() };
		Entries.PushBack({ std::move(CurrentId), std::move(Path) });
	}
	return Entries;
}

TArray<std::string> ListGroup(const std::string_view InGroupPrefix)
{
	TArray<std::string> Paths;
	for ( GroupEntry& Current : ListGroupEntries(InGroupPrefix) )
	{
		Paths.PushBack(std::move(Current.Path));
	}
	return Paths;
}

} // namespace game::config_refs
