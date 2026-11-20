#include "game/EnemyCatalog.h"

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "game/EnemyCatalogLoader.h"

namespace tanks
{

EnemyCatalog& EnemyCatalog::GetInstance()
{
	static EnemyCatalog Instance;
	return Instance;
}

void EnemyCatalog::Init()
{
	Enemies.clear();

	for ( const auto& CurrentConfigId : ConfigReader::GetInstance().GetConfigIds(EnemiesFolder) )
	{
		Enemies.push_back(EnemyCatalogLoader::ReadEnemy(ConfigReader::GetInstance().GetConfig(CurrentConfigId), CurrentConfigId));
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
