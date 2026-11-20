#include "game/EnemyCatalog.h"

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/EnemyCatalogLoader.h"

namespace tanks
{

void EnemyCatalog::Init()
{
	Enemies.clear();

	for ( const auto& CurrentConfigId : GameContext::Get().GetManager<ConfigReader>().GetConfigIds(EnemiesFolder) )
	{
		Enemies.push_back(EnemyCatalogLoader::ReadEnemy(GameContext::Get().GetManager<ConfigReader>().GetConfig(CurrentConfigId), CurrentConfigId));
	}
}

const std::vector<EnemyDef>& EnemyCatalog::GetAll() const
{
	return Enemies;
}

const EnemyDef* EnemyCatalog::Find(std::string_view InId) const
{
	for ( const auto& Enemy : Enemies )
	{
		if ( Enemy.Id == InId )
		{
			return &Enemy;
		}
	}
	return nullptr;
}

std::vector<const EnemyDef*> EnemyCatalog::GetByType(const EnemyType InType) const
{
	std::vector<const EnemyDef*> Result;
	for ( const auto& Enemy : Enemies )
	{
		if ( Enemy.Kind == InType )
		{
			Result.push_back(&Enemy);
		}
	}
	return Result;
}

} // namespace tanks
