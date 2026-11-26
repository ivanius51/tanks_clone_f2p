#include "game/ResourceManifest.h"

#include <algorithm>

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "game/EnemyCatalog.h"
#include "game/objects/TankLoader.h"
#include "model/ConfigNode.h"
#include "services/ResourceManager.h"
#include "templates/TContainers.h"

namespace game
{

namespace
{
	void AddUnique(TArray<std::string>& InList, const std::string& InItem)
	{
		if ( !InItem.empty() && std::find(InList.begin(), InList.end(), InItem) == InList.end() )
		{
			InList.PushBack(InItem);
		}
	}

	void AddSound(TArray<std::string>& InList, const model::ResourceId& InSound)
	{
		if ( InSound.IsValid() )
		{
			AddUnique(InList, std::string(InSound.GetName()));
		}
	}

	void AddTankSounds(TArray<std::string>& InList, const TankDef& InDef)
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
	const model::IConfigNode* FindField(const model::IConfigNode& InConfig, const std::string& InKey)
	{
		const model::IConfigNode* const Field = InConfig.FindChild(InKey);
		if ( !Field )
		{
			return nullptr;
		}
		std::string ReferencedConfig;
		if ( !Field->ReadSelf(ReferencedConfig) )
		{
			return Field;
		}
		return GameContext::Get<ConfigReader>().GetConfig(ReferencedConfig).FindChild(InKey);
	}
}

ResourceManifest::ResourceManifest()
{
	const model::IConfigNode& ConfigData = GameContext::Get<ConfigReader>().GetConfig(ResourcesConfig);

	const model::IConfigNode* const Global = FindField(ConfigData, "global_atlases");
	if ( Global )
	{
		for ( std::size_t Index = 0; Index < Global->GetItemCount(); ++Index )
		{
			std::string CurrentAtlas;
			if ( Global->GetItem(Index)->ReadSelf(CurrentAtlas) )
			{
				AddUnique(GlobalAtlases, CurrentAtlas);
			}
		}
	}

	const model::IConfigNode* const TilesetList = FindField(ConfigData, "tilesets");
	if ( TilesetList )
	{
		for ( const std::string& Key : TilesetList->GetKeys() )
		{
			std::string Value;
			if ( TilesetList->TryRead(Key, Value) )
			{
				Tilesets[Key] = std::move(Value);
			}
		}
	}
	DefaultTileset = ConfigData.GetValueOr("default_tileset", "");
}

const TArray<std::string>& ResourceManifest::GetGlobalAtlases() const
{
	return GlobalAtlases;
}

std::string ResourceManifest::GetTilesetName(const std::string& InTileset) const
{
	return Tilesets.Find(InTileset) ? InTileset : DefaultTileset;
}

std::string ResourceManifest::GetTilesetAtlas(const std::string& InTileset) const
{
	const std::string* const Atlas = Tilesets.Find(GetTilesetName(InTileset));
	return Atlas ? *Atlas : std::string();
}

std::string ResourceManifest::GetTilesetFrame(const std::string& InTileset, const std::string& InFrame)
{
	return InTileset + "/" + InFrame;
}

TArray<std::string> ResourceManifest::GetTilesetAtlases() const
{
	TArray<std::string> Result;
	for ( const auto& CurrentTileset : Tilesets )
	{
		AddUnique(Result, CurrentTileset.Value);
	}
	return Result;
}

TArray<std::string> ResourceManifest::GetLevelAtlases(const LevelData& InLevel) const
{
	TArray<std::string> Result;
	AddUnique(Result, GetTilesetAtlas(InLevel.Tileset));

	for ( const auto& CurrentWave : InLevel.Waves )
	{
		for ( const auto& CurrentEntry : CurrentWave.Entries )
		{
			for ( const auto& CurrentId : LevelLoader::ResolvePool(LevelLoader::GetRule(InLevel, CurrentEntry.Spawner)) )
			{
				if ( const EnemyDef* Enemy = GameContext::Get<EnemyCatalog>().Find(CurrentId) )
				{
					AddUnique(Result, std::string(Enemy->Atlas.View()));
				}
			}
		}
	}
	return Result;
}

TArray<std::string> ResourceManifest::GetLevelSounds(const LevelData& InLevel) const
{
	const ConfigReader& Reader = GameContext::Get<ConfigReader>();
	TArray<std::string> Result;
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

TArray<std::string> ResourceManifest::GetEnemyAtlases() const
{
	TArray<std::string> Result;
	for ( const auto& Enemy : GameContext::Get<EnemyCatalog>().GetAll() )
	{
		AddUnique(Result, std::string(Enemy.Atlas.View()));
	}
	return Result;
}

} // namespace game
