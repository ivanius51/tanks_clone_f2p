#include "services/ConfigRefs.h"
#include "model/Log.h"
#include "services/LevelLegendManager.h"

#include "cocos2d.h"

#include "core/ConfigReader.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "services/ResourceManager.h"

namespace game
{

void LevelLegendManager::Init()
{
	if ( !Legend.FromConfig(config_refs::GetConfig(ids::configs::LevelLegend)) )
	{
		model::Log::Info("LevelLegend: no valid tiles in the legend config");
	}
	for ( const std::string& Error : Legend.GetErrors() )
	{
		model::Log::Info("LevelLegend: %s", Error.c_str());
	}
}

} // namespace game
