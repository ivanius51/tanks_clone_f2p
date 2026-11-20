#include "services/AtlasSet.h"

#include <algorithm>

#include "core/GameContext.h"

namespace tanks
{

AtlasSet::AtlasSet(const std::vector<std::string>& InAtlases)
{
	for ( const std::string& CurrentAtlas : InAtlases )
	{
		Add(CurrentAtlas);
	}
}

bool AtlasSet::Add(const std::string& InAtlas)
{
	if ( std::find(Names.begin(), Names.end(), InAtlas) != Names.end() )
	{
		return true;
	}
	AtlasHandle Handle = GameContext::Get().GetManager<ResourceManager>().AcquireAtlas(InAtlas);
	if ( !Handle )
	{
		return false;
	}
	Handles.push_back(std::move(Handle));
	Names.push_back(InAtlas);
	return true;
}

const std::vector<std::string>& AtlasSet::GetAtlases() const
{
	return Names;
}

} // namespace tanks
