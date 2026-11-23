#include "services/ConfigRefs.h"

#include <algorithm>

#include "core/GameContext.h"
#include "services/ResourceManager.h"

namespace tanks::config_refs
{

std::string ResolveId(const model::ResourceId& InId)
{
	return GameContext::Get().GetManager<ResourceManager>().ResolvePath(InId, model::ResourceKind::Config);
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

std::vector<std::string> ListGroup(const std::string_view InGroupPrefix)
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
	std::vector<std::string> Paths;
	for ( const std::string& CurrentId : Ids )
	{
		Paths.push_back(Resources.ResolvePath(model::ResourceId(CurrentId), model::ResourceKind::Config));
	}
	return Paths;
}

} // namespace tanks::config_refs
