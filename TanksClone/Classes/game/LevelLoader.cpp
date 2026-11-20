#include "game/LevelLoader.h"

#include <algorithm>
#include <sstream>

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "game/EnemyCatalog.h"
#include "game/LevelDataLoader.h"

namespace tanks
{

namespace
{
	constexpr const char* CustomFile = "custom_level.json";
	constexpr const char* StoredCustomId = "custom_level";
	constexpr const char* DefaultEnemy = "Basic";

	std::string CustomPath()
	{
		return cocos2d::FileUtils::getInstance()->getWritablePath() + CustomFile;
	}

	template <typename T>
	bool Contains(const std::vector<T>& InList, const T& InItem)
	{
		return std::find(InList.begin(), InList.end(), InItem) != InList.end();
	}
}

int LevelLoader::GetBuiltinCount()
{
	int CurrentCount = 0;
	while ( ConfigReader::GetInstance().HasConfig(BuiltinId(CurrentCount)) )
	{
		++CurrentCount;
	}
	return CurrentCount;
}

std::string LevelLoader::BuiltinId(const int InIndex)
{
	return std::string(ConfigsFolder) + "levels/level_" + std::to_string(InIndex + 1) + ".json";
}

bool LevelLoader::Load(const std::string& InConfigId, LevelData& InLevel)
{
	if ( !ConfigReader::GetInstance().HasConfig(InConfigId) )
	{
		return false;
	}
	LevelData CurrentLevel;
	if ( !LevelDataLoader::ReadLevel(ConfigReader::GetInstance().GetConfig(InConfigId), CurrentLevel) )
	{
		return false;
	}

	std::vector<std::string> KnownEnemies;
	for ( const std::string& CurrentId : CurrentLevel.EnemyIds )
	{
		if ( EnemyCatalog::GetInstance().Find(CurrentId) )
		{
			KnownEnemies.push_back(CurrentId);
		}
		else
		{
			CCLOGERROR("LevelLoader: unknown enemy %s in %s", CurrentId.c_str(), InConfigId.c_str());
		}
	}
	if ( KnownEnemies.empty() )
	{
		KnownEnemies.push_back(DefaultEnemy);
	}
	CurrentLevel.EnemyIds = KnownEnemies;

	const std::string Problem = Validate(CurrentLevel);
	if ( !Problem.empty() )
	{
		CCLOGERROR("LevelLoader: invalid level %s: %s", InConfigId.c_str(), Problem.c_str());
		return false;
	}
	InLevel = CurrentLevel;
	return true;
}

std::string LevelLoader::CustomId()
{
	return StoredCustomId;
}

bool LevelLoader::HasCustom()
{
	return cocos2d::FileUtils::getInstance()->isFileExist(CustomPath());
}

bool LevelLoader::LoadCustom(LevelData& InLevel)
{
	return ConfigReader::GetInstance().LoadConfigFile(CustomPath(), StoredCustomId) && Load(StoredCustomId, InLevel);
}

bool LevelLoader::SaveCustom(const LevelData& InLevel)
{
	return cocos2d::FileUtils::getInstance()->writeStringToFile(ToJson(InLevel), CustomPath());
}

LevelData LevelLoader::CreateEmpty()
{
	LevelData CurrentLevel;
	CurrentLevel.Name = "CUSTOM_LEVEL";
	CurrentLevel.Rows.assign(GridRows, std::string(GridColumns, tiles::Empty));
	CurrentLevel.EnemyIds.push_back(DefaultEnemy);
	return CurrentLevel;
}

std::string LevelLoader::Validate(const LevelData& InLevel)
{
	if ( static_cast<int>(InLevel.Rows.size()) != GridRows )
	{
		return "LEVEL_ERROR_SIZE";
	}

	int Players = 0;
	std::string SpawnerTiles;
	for ( const auto& Row : InLevel.Rows )
	{
		if ( static_cast<int>(Row.size()) != GridColumns )
		{
			return "LEVEL_ERROR_SIZE";
		}
		for ( char CurrentTile : Row )
		{
			if ( CurrentTile == tiles::PlayerSpawn )
			{
				++Players;
			}
			else if ( tiles::IsSpawner(CurrentTile) )
			{
				SpawnerTiles += CurrentTile;
			}
			else if ( CurrentTile != tiles::Empty && CurrentTile != tiles::Brick && CurrentTile != tiles::Stone && CurrentTile != tiles::Wood
				&& CurrentTile != tiles::Ammo && CurrentTile != tiles::Repair )
			{
				return "LEVEL_ERROR_TILE";
			}
		}
	}

	if ( Players != 1 )
	{
		return "LEVEL_ERROR_PLAYER";
	}
	if ( SpawnerTiles.empty() || InLevel.TotalEnemies < 1 || InLevel.MaxAlive < 1 )
	{
		return "LEVEL_ERROR_ENEMY";
	}

	// Waves may only use spawners that are on the map.
	for ( const auto& CurrentWave : InLevel.Waves )
	{
		for ( const auto& CurrentEntry : CurrentWave.Entries )
		{
			if ( SpawnerTiles.find(CurrentEntry.Spawner) == std::string::npos )
			{
				return "LEVEL_ERROR_ENEMY";
			}
		}
	}
	return "";
}

std::vector<std::string> LevelLoader::ResolvePool(const SpawnRule& InRule)
{
	const bool bRestricted = !InRule.Types.empty() || !InRule.Enemies.empty();

	std::vector<std::string> Result;
	for ( const auto& Enemy : EnemyCatalog::GetInstance().GetAll() )
	{
		const bool bIncluded = !bRestricted || Contains(InRule.Enemies, Enemy.Id) || Contains(InRule.Types, Enemy.Kind);
		const bool bExcluded = Contains(InRule.ExcludeEnemies, Enemy.Id) || Contains(InRule.ExcludeTypes, Enemy.Kind);
		if ( bIncluded && !bExcluded )
		{
			Result.push_back(Enemy.Id);
		}
	}
	return Result;
}

SpawnRule LevelLoader::GetRule(const LevelData& InLevel, const char InSpawner)
{
	auto RuleIt = InLevel.Spawners.find(InSpawner);
	if ( RuleIt != InLevel.Spawners.end() )
	{
		return RuleIt->second;
	}

	// An undescribed spawner sends the enemies listed in the simple form of the level.
	SpawnRule Rule;
	Rule.Enemies = InLevel.EnemyIds;
	return Rule;
}

int LevelLoader::GetTotalEnemies(const LevelData& InLevel)
{
	int Total = 0;
	for ( const auto& CurrentWave : InLevel.Waves )
	{
		for ( const auto& CurrentEntry : CurrentWave.Entries )
		{
			Total += CurrentEntry.Count;
		}
	}
	return Total;
}

std::string LevelLoader::ToJson(const LevelData& InLevel)
{
	// Only the simple form is written: that is what the editor edits.
	std::ostringstream Json;
	Json << "{\n";
	Json << "\t\"name\": \"" << InLevel.Name << "\",\n";
	if ( !InLevel.Tileset.empty() )
	{
		Json << "\t\"tileset\": \"" << InLevel.Tileset << "\",\n";
	}
	Json << "\t\"rows\": [\n";
	for ( size_t Index = 0; Index < InLevel.Rows.size(); ++Index )
	{
		Json << "\t\t\"" << InLevel.Rows[Index] << "\"" << (Index + 1 < InLevel.Rows.size() ? "," : "") << "\n";
	}
	Json << "\t],\n";
	Json << "\t\"enemies\": {\n";
	Json << "\t\t\"total\": " << InLevel.TotalEnemies << ",\n";
	Json << "\t\t\"max_alive\": " << InLevel.MaxAlive << ",\n";
	Json << "\t\t\"spawn_interval\": " << InLevel.SpawnInterval << ",\n";
	Json << "\t\t\"types\": [";
	for ( size_t Index = 0; Index < InLevel.EnemyIds.size(); ++Index )
	{
		Json << "\"" << InLevel.EnemyIds[Index] << "\"" << (Index + 1 < InLevel.EnemyIds.size() ? ", " : "");
	}
	Json << "]\n";
	Json << "\t}\n";
	Json << "}\n";
	return Json.str();
}

} // namespace tanks
