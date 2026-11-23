#include "game/LevelLoader.h"

#include <algorithm>

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "ports/IStorage.h"
#include "game/EnemyCatalog.h"
#include "game/LevelDataLoader.h"
#include "model/CampaignList.h"
#include "services/ConfigRefs.h"
#include "services/LevelLegendManager.h"

namespace tanks
{

namespace
{
	constexpr const char* CustomFile = "custom_level.json";
	constexpr const char* StoredCustomId = "custom_level";
	constexpr const char* DefaultEnemy = "Basic";

	template <typename T>
	bool Contains(const std::vector<T>& InList, const T& InItem)
	{
		return std::find(InList.begin(), InList.end(), InItem) != InList.end();
	}
}

namespace
{
	std::vector<std::string> CampaignLevelPaths()
	{
		std::vector<std::string> Paths;
		for ( const std::string& Id : model::CampaignList::FromConfig(config_refs::GetConfig(ids::configs::Campaign)).GetLevelIds() )
		{
			const std::string Path = config_refs::ResolveName(Id);
			if ( !Path.empty() )
			{
				Paths.push_back(Path);
			}
		}
		if ( Paths.empty() )
		{
			CCLOGERROR("LevelLoader: the campaign list is empty, using every level id in order");
			return config_refs::ListGroup(ids::config_groups::Levels);
		}
		return Paths;
	}
}

int LevelLoader::GetBuiltinCount()
{
	return static_cast<int>(CampaignLevelPaths().size());
}

std::string LevelLoader::BuiltinId(const int InIndex)
{
	const std::vector<std::string> Levels = CampaignLevelPaths();
	return InIndex >= 0 && InIndex < static_cast<int>(Levels.size()) ? Levels[InIndex] : std::string();
}

bool LevelLoader::Load(const std::string& InConfigId, LevelData& InLevel)
{
	if ( !GameContext::Get().GetManager<ConfigReader>().HasConfig(InConfigId) )
	{
		return false;
	}
	LevelData CurrentLevel;
	if ( !LevelDataLoader::ReadLevel(GameContext::Get().GetManager<ConfigReader>().GetConfig(InConfigId), CurrentLevel) )
	{
		return false;
	}

	std::vector<std::string> KnownEnemies;
	for ( const std::string& CurrentId : CurrentLevel.EnemyIds )
	{
		if ( GameContext::Get().GetManager<EnemyCatalog>().Find(CurrentId) )
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
	return GameContext::Get().GetStorage().Exists(CustomFile);
}

bool LevelLoader::LoadCustom(LevelData& InLevel)
{
	const std::optional<std::string> Text = GameContext::Get().GetStorage().ReadText(CustomFile);
	return Text && GameContext::Get().GetManager<ConfigReader>().LoadConfigText(*Text, StoredCustomId) && Load(StoredCustomId, InLevel);
}

bool LevelLoader::SaveCustom(const LevelData& InLevel)
{
	return GameContext::Get().GetStorage().WriteText(CustomFile, ToJson(InLevel));
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
	return Validate(InLevel, GameContext::Get().GetManager<LevelLegendManager>().Get());
}

std::string LevelLoader::Validate(const LevelData& InLevel, const model::LevelLegend& InLegend)
{
	if ( static_cast<int>(InLevel.Rows.size()) != GridRows )
	{
		return "LEVEL_ERROR_SIZE";
	}
	for ( const auto& Row : InLevel.Rows )
	{
		if ( static_cast<int>(Row.size()) != GridColumns )
		{
			return "LEVEL_ERROR_SIZE";
		}
	}

	const model::LegendScan Scan = InLegend.Scan(InLevel.Rows);
	if ( Scan.bUnknownTile || Scan.bUniqueRepeated )
	{
		return "LEVEL_ERROR_TILE";
	}
	if ( Scan.Players.size() != 1 )
	{
		return "LEVEL_ERROR_PLAYER";
	}
	if ( Scan.Spawners.empty() || InLevel.TotalEnemies < 1 || InLevel.MaxAlive < 1 )
	{
		return "LEVEL_ERROR_ENEMY";
	}

	// Waves may only use spawners that are on the map.
	for ( const auto& CurrentWave : InLevel.Waves )
	{
		for ( const auto& CurrentEntry : CurrentWave.Entries )
		{
			if ( Scan.SpawnerTiles.find(CurrentEntry.Spawner) == std::string::npos )
			{
				return "LEVEL_ERROR_ENEMY";
			}
		}
	}

	const model::GridPath Grid = BuildPathGrid(InLevel, InLegend);
	for ( const model::GridCell& Spawner : Scan.Spawners )
	{
		if ( !Grid.IsReachable(Spawner, Scan.Players.front()) )
		{
			return "LEVEL_ERROR_BLOCKED";
		}
	}
	return "";
}

model::GridPath LevelLoader::BuildPathGrid(const LevelData& InLevel)
{
	return BuildPathGrid(InLevel, GameContext::Get().GetManager<LevelLegendManager>().Get());
}

model::GridPath LevelLoader::BuildPathGrid(const LevelData& InLevel, const model::LevelLegend& InLegend)
{
	return InLegend.BuildPathGrid(InLevel.Rows, GridColumns);
}

std::vector<std::string> LevelLoader::ResolvePool(const SpawnRule& InRule)
{
	const bool bRestricted = !InRule.Types.empty() || !InRule.Enemies.empty();

	std::vector<std::string> Result;
	for ( const auto& Enemy : GameContext::Get().GetManager<EnemyCatalog>().GetAll() )
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

bool LevelLoader::HasOwnPools(const LevelData& InLevel)
{
	return !InLevel.Spawners.empty() || LevelDataLoader::HasAuthoredWaves(InLevel);
}

std::string LevelLoader::ToJson(const LevelData& InLevel)
{
	return LevelDataLoader::WriteLevel(InLevel);
}

} // namespace tanks
