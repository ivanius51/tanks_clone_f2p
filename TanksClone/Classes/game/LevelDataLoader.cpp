#include "game/LevelDataLoader.h"

#include <string>

#include "cocos2d.h"

namespace tanks
{

namespace
{
	constexpr float DefaultWaveDelay = 1.0f;
}

bool LevelDataLoader::ReadLevel(const model::IConfigNode& InNode, LevelData& OutLevel)
{
	LevelData CurrentLevel;
	if ( !InNode.TryRead("rows", CurrentLevel.Rows) )
	{
		return false;
	}
	CurrentLevel.Name = InNode.GetValueOr("name", "");
	CurrentLevel.Tileset = InNode.GetValueOr("tileset", "");
	CurrentLevel.AiFeatures = model::AiFeatureSet::ReadSwitches(InNode);
	if ( const model::IConfigNode* const ScriptNode = InNode.FindChild("script") )
	{
		CurrentLevel.Script = model::Script::FromConfig(*ScriptNode);
	}

	if ( const model::IConfigNode* const Enemies = InNode.FindChild("enemies") )
	{
		CurrentLevel.TotalEnemies = Enemies->GetValueOr("total", CurrentLevel.TotalEnemies);
		CurrentLevel.MaxAlive = Enemies->GetValueOr("max_alive", CurrentLevel.MaxAlive);
		CurrentLevel.SpawnInterval = Enemies->GetValueOr("spawn_interval", CurrentLevel.SpawnInterval);
		Enemies->TryRead("types", CurrentLevel.EnemyIds);
	}

	if ( const model::IConfigNode* const Spawners = InNode.FindChild("spawners") )
	{
		for ( const std::string& Key : Spawners->GetKeys() )
		{
			if ( Key.size() == 1 )
			{
				CurrentLevel.Spawners[Key[0]] = ReadRule(*Spawners->FindChild(Key));
			}
		}
	}

	if ( const model::IConfigNode* const Waves = InNode.FindChild("waves") )
	{
		for ( std::size_t Index = 0; Index < Waves->GetItemCount(); ++Index )
		{
			CurrentLevel.Waves.push_back(ReadWave(*Waves->GetItem(Index)));
		}
	}
	if ( CurrentLevel.Waves.empty() )
	{
		Wave CurrentWave;
		CurrentWave.Delay = DefaultWaveDelay;
		CurrentWave.MaxAlive = CurrentLevel.MaxAlive;
		CurrentWave.Interval = CurrentLevel.SpawnInterval;
		CurrentWave.Entries.push_back({ tiles::EnemySpawn, CurrentLevel.TotalEnemies });
		CurrentLevel.Waves.push_back(CurrentWave);
	}

	OutLevel = CurrentLevel;
	return true;
}

SpawnRule LevelDataLoader::ReadRule(const model::IConfigNode& InNode)
{
	SpawnRule Rule;
	Rule.Types = ReadEnemyTypes(InNode, "types");
	InNode.TryRead("enemies", Rule.Enemies);
	Rule.ExcludeTypes = ReadEnemyTypes(InNode, "exclude_types");
	InNode.TryRead("exclude", Rule.ExcludeEnemies);
	return Rule;
}

Wave LevelDataLoader::ReadWave(const model::IConfigNode& InNode)
{
	Wave CurrentWave;
	CurrentWave.Delay = InNode.GetValueOr("delay", CurrentWave.Delay);
	CurrentWave.MaxAlive = InNode.GetValueOr("max_alive", CurrentWave.MaxAlive);
	CurrentWave.Interval = InNode.GetValueOr("interval", CurrentWave.Interval);
	if ( const model::IConfigNode* const Spawns = InNode.FindChild("spawns") )
	{
		for ( std::size_t Index = 0; Index < Spawns->GetItemCount(); ++Index )
		{
			const model::IConfigNode& Item = *Spawns->GetItem(Index);
			const std::string SpawnerId = Item.GetValueOr("spawner", std::string(1, tiles::EnemySpawn));
			WaveEntry Entry;
			Entry.Spawner = SpawnerId.empty() ? tiles::EnemySpawn : SpawnerId[0];
			Entry.Count = Item.GetValueOr("count", Entry.Count);
			CurrentWave.Entries.push_back(Entry);
		}
	}
	return CurrentWave;
}

std::vector<EnemyType> LevelDataLoader::ReadEnemyTypes(const model::IConfigNode& InNode, const std::string_view InKey)
{
	std::vector<std::string> Names;
	InNode.TryRead(InKey, Names);

	std::vector<EnemyType> Types;
	for ( const std::string& Name : Names )
	{
		EnemyType Type = EnemyType::Medium;
		if ( EnumFromName(std::string_view(Name), Type) )
		{
			Types.push_back(Type);
		}
		else
		{
			CCLOGERROR("LevelLoader: unknown enemy class %s", Name.c_str());
		}
	}
	return Types;
}

} // namespace tanks
