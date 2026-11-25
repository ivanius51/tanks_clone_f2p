#include "game/ResourceManifest.h"

#include <algorithm>

#include "core/ConfigReader.h"
#include "core/DataConverter.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/EnemyCatalog.h"
#include "game/objects/TankLoader.h"
#include "core/ResourceIds.h"
#include "services/ResourceManager.h"

namespace game
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

	void AddSound(std::vector<std::string>& InList, const model::ResourceId& InSound)
	{
		if ( InSound.IsValid() )
		{
			AddUnique(InList, std::string(InSound.GetName()));
		}
	}

	void AddTankSounds(std::vector<std::string>& InList, const TankDef& InDef)
	{
		AddSound(InList, InDef.HitSound);
		AddSound(InList, InDef.ArmorSound);
		AddSound(InList, InDef.ExplodeSound);
		for ( const WeaponDef& CurrentWeapon : InDef.Weapons )
		{
			AddSound(InList, CurrentWeapon.FireSound);
		}
		for ( const AbilityDef& CurrentAbility : InDef.Abilities )
		{
			AddSound(InList, CurrentAbility.ActivateSound);
		}
	}

	// A field is either the value itself or the path of a config that holds it under the same key.
	const cocos2d::Value* FindField(const cocos2d::ValueMap& InConfig, const std::string& InKey)
	{
		const auto FieldIt = InConfig.find(InKey);
		if ( FieldIt == InConfig.end() )
		{
			return nullptr;
		}
		if ( FieldIt->second.getType() != cocos2d::Value::Type::STRING )
		{
			return &FieldIt->second;
		}
		const cocos2d::ValueMap& Referenced = GameContext::Get<ConfigReader>().GetObjectData(FieldIt->second.asString()).Values;
		const auto ReferencedIt = Referenced.find(InKey);
		return ReferencedIt != Referenced.end() ? &ReferencedIt->second : nullptr;
	}
}

ResourceManifest::ResourceManifest()
{
	const cocos2d::ValueMap& ConfigData = GameContext::Get<ConfigReader>().GetObjectData(ResourcesConfig).Values;

	const cocos2d::Value* const Global = FindField(ConfigData, "global_atlases");
	if ( Global && Global->getType() == cocos2d::Value::Type::VECTOR )
	{
		for ( const auto& CurrentAtlas : Global->asValueVector() )
		{
			AddUnique(GlobalAtlases, CurrentAtlas.asString());
		}
	}

	const cocos2d::Value* const TilesetList = FindField(ConfigData, "tilesets");
	if ( TilesetList && TilesetList->getType() == cocos2d::Value::Type::MAP )
	{
		for ( const auto& CurrentTileset : TilesetList->asValueMap() )
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
				if ( const EnemyDef* Enemy = GameContext::Get<EnemyCatalog>().Find(CurrentId) )
				{
					AddUnique(Result, Enemy->Atlas);
				}
			}
		}
	}
	return Result;
}

std::vector<std::string> ResourceManifest::GetLevelSounds(const LevelData& InLevel) const
{
	const ConfigReader& Reader = GameContext::Get<ConfigReader>();
	std::vector<std::string> Result;
	const FPersistentString PlayerConfig = GameContext::Get<ResourceManager>().ResolvePath(ids::configs::PlayerTank, model::ResourceKind::Config);
	AddTankSounds(Result, TankLoader::ReadTank(Reader.GetConfig(PlayerConfig)));

	for ( const auto& CurrentWave : InLevel.Waves )
	{
		for ( const auto& CurrentEntry : CurrentWave.Entries )
		{
			for ( const auto& CurrentId : LevelLoader::ResolvePool(LevelLoader::GetRule(InLevel, CurrentEntry.Spawner)) )
			{
				if ( const EnemyDef* Enemy = GameContext::Get<EnemyCatalog>().Find(CurrentId) )
				{
					AddTankSounds(Result, TankLoader::ReadTank(Reader.GetConfig(Enemy->ConfigId)));
				}
			}
		}
	}
	return Result;
}

ResourceSet ResourceManifest::GetLevelResources(const LevelData& InLevel, const bool bInWithSounds) const
{
	ResourceSet Result;
	Result.Atlases = GetLevelAtlases(InLevel);
	if ( bInWithSounds )
	{
		Result.Sounds = GetLevelSounds(InLevel);
	}
	return Result;
}

std::vector<std::string> ResourceManifest::GetEnemyAtlases() const
{
	std::vector<std::string> Result;
	for ( const auto& Enemy : GameContext::Get<EnemyCatalog>().GetAll() )
	{
		AddUnique(Result, Enemy.Atlas);
	}
	return Result;
}

} // namespace game
