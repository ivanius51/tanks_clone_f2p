#include "services/LevelLegendManager.h"

#include "cocos2d.h"

#include "core/ConfigReader.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "services/ResourceManager.h"

namespace tanks
{

void LevelLegendManager::Init()
{
	const std::string Path = GameContext::Get().GetManager<ResourceManager>().ResolvePath(ids::configs::LevelLegend, model::ResourceKind::Config);
	if ( !Legend.FromConfig(GameContext::Get().GetManager<ConfigReader>().GetConfig(Path)) )
	{
		cocos2d::log("LevelLegend: no valid tiles in %s", Path.c_str());
	}
	for ( const std::string& Error : Legend.GetErrors() )
	{
		cocos2d::log("LevelLegend: %s", Error.c_str());
	}
}

} // namespace tanks
