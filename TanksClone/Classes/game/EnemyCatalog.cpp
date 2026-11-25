#include "game/EnemyCatalog.h"

#include "core/ConfigReader.h"
#include "core/ResourceIds.h"
#include "core/GameContext.h"
#include "game/EnemyCatalogLoader.h"
#include "services/ConfigRefs.h"

namespace game
{

void EnemyCatalog::Init()
{
	Enemies.Clear();

	for ( const auto& CurrentConfigId : config_refs::ListGroup(ids::config_groups::Enemies) )
	{
		Enemies.Add(EnemyCatalogLoader::ReadEnemy(GameContext::Get<ConfigReader>().GetConfig(CurrentConfigId), CurrentConfigId));
	}
}

const std::vector<EnemyDef>& EnemyCatalog::GetAll() const
{
	return Enemies.GetAll();
}

const EnemyDef* EnemyCatalog::Find(std::string_view InId) const
{
	return Enemies.Find(InId);
}

std::vector<const EnemyDef*> EnemyCatalog::GetByType(const EnemyType InType) const
{
	std::vector<const EnemyDef*> Result;
	for ( const auto& Enemy : Enemies.GetAll() )
	{
		if ( Enemy.Kind == InType )
		{
			Result.push_back(&Enemy);
		}
	}
	return Result;
}

} // namespace game
