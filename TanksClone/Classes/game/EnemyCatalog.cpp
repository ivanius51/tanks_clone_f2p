#include "game/EnemyCatalog.h"

#include "core/ConfigReader.h"
#include "core/DataConverter.h"
#include "core/GameConstants.h"

#include <string_view>

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

	const std::string Prefix = EnemiesFolder;
	const std::string Extension = ".json";
	for ( const auto& CurrentConfigId : ConfigReader::GetInstance().GetConfigIds(Prefix) )
	{
		EnemyDef CurrentDef;
		CurrentDef.ConfigId = CurrentConfigId;
		CurrentDef.Id = CurrentConfigId.substr(Prefix.size(), CurrentConfigId.size() - Prefix.size() - Extension.size());

		const cocos2d::ValueMap& ConfigData = ConfigReader::GetInstance().GetObjectData(CurrentConfigId).Values;
		CurrentDef.NameKey = DataConverter::GetString(ConfigData, "name", CurrentDef.Id);
		CurrentDef.Atlas = DataConverter::GetString(ConfigData, "atlas", "");
		auto TypeIt = ConfigData.find("enemy_type");
		if ( TypeIt != ConfigData.end() )
		{
			CurrentDef.Kind = ParseEnemyType(TypeIt->second);
		}
		Enemies.push_back(CurrentDef);
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
