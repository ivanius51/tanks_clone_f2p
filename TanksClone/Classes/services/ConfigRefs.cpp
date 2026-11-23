#include "services/ConfigRefs.h"

#include <algorithm>

#include "core/ConfigReader.h"
#include "core/GameContext.h"
#include "services/ResourceManager.h"

namespace tanks::config_refs
{

std::string ResolveId(const model::ResourceId& InId)
{
	return GameContext::Get().GetManager<ResourceManager>().ResolvePath(InId, model::ResourceKind::Config);
}

const model::IConfigNode& GetConfig(const model::ResourceId& InId)
{
	return GameContext::Get().GetManager<ConfigReader>().GetConfig(ResolveId(InId));
}

const model::IConfigNode* FindChild(const model::ResourceId& InId, const std::string_view InKey)
{
	return GetConfig(InId).FindChild(InKey);
}

std::string ResolveName(const std::string_view InName)
{
	if ( InName.empty() )
	{
		return std::string();
	}
	ResourceManager& Resources = GameContext::Get().GetManager<ResourceManager>();
	const model::ResourceId Id = Resources.FindId(InName, model::ResourceKind::Config);
	return Id.IsValid() ? Resources.ResolvePath(Id, model::ResourceKind::Config) : std::string();
}

std::vector<GroupEntry> ListGroupEntries(const std::string_view InGroupPrefix)
{
	ResourceManager& Resources = GameContext::Get().GetManager<ResourceManager>();
	std::vector<std::string> Ids;
	for ( std::string& CurrentId : Resources.GetTable().GetIds(model::ResourceKind::Config) )
	{
		if ( CurrentId.compare(0, InGroupPrefix.size(), InGroupPrefix) == 0 )
		{
			Ids.push_back(std::move(CurrentId));
		}
	}
	std::sort(Ids.begin(), Ids.end(), [](const std::string& InLeft, const std::string& InRight)
	{
		return InLeft.size() != InRight.size() ? InLeft.size() < InRight.size() : InLeft < InRight;
	});
	std::vector<GroupEntry> Entries;
	for ( std::string& CurrentId : Ids )
	{
		std::string Path = Resources.ResolvePath(model::ResourceId(CurrentId), model::ResourceKind::Config);
		Entries.push_back({ std::move(CurrentId), std::move(Path) });
	}
	return Entries;
}

std::vector<std::string> ListGroup(const std::string_view InGroupPrefix)
{
	std::vector<std::string> Paths;
	for ( GroupEntry& Current : ListGroupEntries(InGroupPrefix) )
	{
		Paths.push_back(std::move(Current.Path));
	}
	return Paths;
}

} // namespace tanks::config_refs
