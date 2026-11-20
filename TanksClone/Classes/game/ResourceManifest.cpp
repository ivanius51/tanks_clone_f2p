#include "game/ResourceManifest.h"

#include <algorithm>
#include "core/ConfigReader.h"
#include "core/DataConverter.h"
#include "core/GameConstants.h"
#include "game/EnemyCatalog.h"

namespace tanks
{

namespace
{
	void AddUnique(std::vector<std::string>& InList, const std::string& InItem)
	{
		if ( !InItem.empty() && std::find(InList.begin(), InList.end(), InItem) == InList.end() )
		{
			InList.push_back(InItem);
		}
	}
}

ResourceManifest::ResourceManifest()
{
	const cocos2d::ValueMap& ConfigData = ConfigReader::GetInstance().GetObjectData(ResourcesConfig).Values;

	auto GlobalIt = ConfigData.find("global_atlases");
	if ( GlobalIt != ConfigData.end() && GlobalIt->second.getType() == cocos2d::Value::Type::VECTOR )
	{
		for ( const auto& CurrentAtlas : GlobalIt->second.asValueVector() )
		{
			AddUnique(GlobalAtlases, CurrentAtlas.asString());
		}
	}

	auto TilesetsIt = ConfigData.find("tilesets");
	if ( TilesetsIt != ConfigData.end() && TilesetsIt->second.getType() == cocos2d::Value::Type::MAP )
	{
		for ( const auto& CurrentTileset : TilesetsIt->second.asValueMap() )
		{
			Tilesets[CurrentTileset.first] = CurrentTileset.second.asString();
		}
	}
	DefaultTileset = DataConverter::GetString(ConfigData, "default_tileset", "");
}

const std::vector<std::string>& ResourceManifest::GetGlobalAtlases() const
{
	return GlobalAtlases;
}

std::string ResourceManifest::GetTilesetName(const std::string& InTileset) const
{
	return Tilesets.count(InTileset) ? InTileset : DefaultTileset;
}

std::string ResourceManifest::GetTilesetAtlas(const std::string& InTileset) const
{
	auto TilesetIt = Tilesets.find(GetTilesetName(InTileset));
	return TilesetIt != Tilesets.end() ? TilesetIt->second : std::string();
}

std::string ResourceManifest::GetTilesetFrame(const std::string& InTileset, const std::string& InFrame)
{
	return InTileset + "/" + InFrame;
}

std::vector<std::string> ResourceManifest::GetTilesetAtlases() const
{
	std::vector<std::string> Result;
	for ( const auto& CurrentTileset : Tilesets )
	{
		AddUnique(Result, CurrentTileset.second);
	}
	return Result;
}

std::vector<std::string> ResourceManifest::GetLevelAtlases(const LevelData& InLevel) const
{
	std::vector<std::string> Result;
	AddUnique(Result, GetTilesetAtlas(InLevel.Tileset));

	for ( const auto& CurrentWave : InLevel.Waves )
	{
		for ( const auto& CurrentEntry : CurrentWave.Entries )
		{
			for ( const auto& CurrentId : LevelLoader::ResolvePool(LevelLoader::GetRule(InLevel, CurrentEntry.Spawner)) )
			{
				if ( const EnemyDef* Enemy = EnemyCatalog::GetInstance().Find(CurrentId) )
				{
					AddUnique(Result, Enemy->Atlas);
				}
			}
		}
	}
	return Result;
}

std::vector<std::string> ResourceManifest::GetEnemyAtlases() const
{
	std::vector<std::string> Result;
	for ( const auto& Enemy : EnemyCatalog::GetInstance().GetAll() )
	{
		AddUnique(Result, Enemy.Atlas);
	}
	return Result;
}

} // namespace tanks
