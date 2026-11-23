#include "game/LevelDataLoader.h"

#include <string>

#include "cocos2d.h"

#include "model/LevelJsonWriter.h"

namespace tanks
{

namespace
{
	constexpr float DefaultWaveDelay = 1.0f;

	model::JsonValue Number(const float InValue)
	{
		return model::LevelJsonWriter::FromFloat(InValue);
	}

	model::JsonValue Strings(const std::vector<std::string>& InItems)
	{
		model::JsonValue::Array Items;
		for ( const std::string& Item : InItems )
		{
			Items.emplace_back(Item);
		}
		return model::JsonValue(std::move(Items));
	}

	model::JsonValue TypeNames(const std::vector<EnemyType>& InTypes)
	{
		model::JsonValue::Array Items;
		for ( const EnemyType Type : InTypes )
		{
			Items.emplace_back(EnemyTypeName(Type));
		}
		return model::JsonValue(std::move(Items));
	}

	model::JsonValue RuleToJson(const SpawnRule& InRule)
	{
		model::JsonValue::Object Members;
		if ( !InRule.Types.empty() )
		{
			Members.emplace("types", TypeNames(InRule.Types));
		}
		if ( !InRule.Enemies.empty() )
		{
			Members.emplace("enemies", Strings(InRule.Enemies));
		}
		if ( !InRule.ExcludeTypes.empty() )
		{
			Members.emplace("exclude_types", TypeNames(InRule.ExcludeTypes));
		}
		if ( !InRule.ExcludeEnemies.empty() )
		{
			Members.emplace("exclude", Strings(InRule.ExcludeEnemies));
		}
		return model::JsonValue(std::move(Members));
	}

	model::JsonValue WaveToJson(const Wave& InWave)
	{
		model::JsonValue::Array Spawns;
		for ( const WaveEntry& Entry : InWave.Entries )
		{
			model::JsonValue::Object Item;
			Item.emplace("spawner", model::JsonValue(std::string(1, Entry.Spawner)));
			Item.emplace("count", model::JsonValue(Entry.Count));
			Spawns.emplace_back(std::move(Item));
		}
		model::JsonValue::Object Members;
		Members.emplace("delay", Number(InWave.Delay));
		Members.emplace("max_alive", model::JsonValue(InWave.MaxAlive));
		Members.emplace("interval", Number(InWave.Interval));
		Members.emplace("spawns", model::JsonValue(std::move(Spawns)));
		return model::JsonValue(std::move(Members));
	}

	bool SameWave(const Wave& InFirst, const Wave& InSecond)
	{
		if ( InFirst.Delay != InSecond.Delay || InFirst.MaxAlive != InSecond.MaxAlive || InFirst.Interval != InSecond.Interval
			|| InFirst.Entries.size() != InSecond.Entries.size() )
		{
			return false;
		}
		for ( size_t Index = 0; Index < InFirst.Entries.size(); ++Index )
		{
			if ( InFirst.Entries[Index].Spawner != InSecond.Entries[Index].Spawner || InFirst.Entries[Index].Count != InSecond.Entries[Index].Count )
			{
				return false;
			}
		}
		return true;
	}
}

Wave LevelDataLoader::MakeSimpleWave(const LevelData& InLevel)
{
	Wave SimpleWave;
	SimpleWave.Delay = DefaultWaveDelay;
	SimpleWave.MaxAlive = InLevel.MaxAlive;
	SimpleWave.Interval = InLevel.SpawnInterval;
	SimpleWave.Entries.push_back({ tiles::EnemySpawn, InLevel.TotalEnemies });
	return SimpleWave;
}

bool LevelDataLoader::HasAuthoredWaves(const LevelData& InLevel)
{
	if ( InLevel.Waves.empty() )
	{
		return false;
	}
	return !(InLevel.Waves.size() == 1 && SameWave(InLevel.Waves.front(), MakeSimpleWave(InLevel)));
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
	CurrentLevel.Music = InNode.GetValueOr("music", "");
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
		CurrentLevel.Waves.push_back(MakeSimpleWave(CurrentLevel));
	}

	if ( const model::IConfigNode* const Fog = InNode.FindChild("fog_of_war") )
	{
		CurrentLevel.Fog.bEnabled = Fog->GetValueOr("enabled", CurrentLevel.Fog.bEnabled);
		CurrentLevel.Fog.VisionTiles = Fog->GetValueOr("vision_tiles", CurrentLevel.Fog.VisionTiles);
		CurrentLevel.Fog.bShowInitialWalls = Fog->GetValueOr("show_initial_walls", CurrentLevel.Fog.bShowInitialWalls);
	}

	model::JsonValue::Object Unknown;
	for ( const std::string& Key : InNode.GetKeys() )
	{
		if ( !model::LevelJsonWriter::IsKnownKey(Key) )
		{
			Unknown.emplace(Key, model::LevelJsonWriter::FromNode(*InNode.FindChild(Key)));
		}
	}
	if ( !Unknown.empty() )
	{
		CurrentLevel.Extra = model::JsonValue(std::move(Unknown));
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

std::string LevelDataLoader::WriteLevel(const LevelData& InLevel)
{
	model::JsonValue::Object Root;
	if ( InLevel.Extra.IsObject() )
	{
		Root = InLevel.Extra.GetMembers();
	}
	Root.insert_or_assign("name", model::JsonValue(InLevel.Name));
	if ( !InLevel.Music.empty() )
	{
		Root.insert_or_assign("music", model::JsonValue(InLevel.Music));
	}
	if ( !InLevel.Tileset.empty() )
	{
		Root.insert_or_assign("tileset", model::JsonValue(InLevel.Tileset));
	}
	if ( !(InLevel.Fog == FogOfWar()) )
	{
		model::JsonValue::Object Fog;
		Fog.emplace("enabled", model::JsonValue(InLevel.Fog.bEnabled));
		Fog.emplace("vision_tiles", Number(InLevel.Fog.VisionTiles));
		Fog.emplace("show_initial_walls", model::JsonValue(InLevel.Fog.bShowInitialWalls));
		Root.insert_or_assign("fog_of_war", model::JsonValue(std::move(Fog)));
	}
	Root.insert_or_assign("rows", Strings(InLevel.Rows));

	if ( !HasAuthoredWaves(InLevel) || !InLevel.EnemyIds.empty() )
	{
		model::JsonValue::Object Enemies;
		Enemies.emplace("total", model::JsonValue(InLevel.TotalEnemies));
		Enemies.emplace("max_alive", model::JsonValue(InLevel.MaxAlive));
		Enemies.emplace("spawn_interval", Number(InLevel.SpawnInterval));
		Enemies.emplace("types", Strings(InLevel.EnemyIds));
		Root.insert_or_assign("enemies", model::JsonValue(std::move(Enemies)));
	}

	if ( !InLevel.Spawners.empty() )
	{
		model::JsonValue::Object Spawners;
		for ( const auto& Spawner : InLevel.Spawners )
		{
			Spawners.emplace(std::string(1, Spawner.first), RuleToJson(Spawner.second));
		}
		Root.insert_or_assign("spawners", model::JsonValue(std::move(Spawners)));
	}
	if ( HasAuthoredWaves(InLevel) )
	{
		model::JsonValue::Array Waves;
		for ( const Wave& CurrentWave : InLevel.Waves )
		{
			Waves.push_back(WaveToJson(CurrentWave));
		}
		Root.insert_or_assign("waves", model::JsonValue(std::move(Waves)));
	}
	if ( !InLevel.AiFeatures.empty() )
	{
		model::JsonValue::Object Switches;
		for ( const auto& Switch : InLevel.AiFeatures )
		{
			Switches.emplace(Switch.first, model::JsonValue(Switch.second));
		}
		Root.insert_or_assign("ai_features", model::JsonValue(std::move(Switches)));
	}
	if ( !InLevel.Script.IsEmpty() || !InLevel.Script.Actors.empty() )
	{
		Root.insert_or_assign("script", InLevel.Script.ToJson());
	}
	return model::LevelJsonWriter::Write(model::JsonValue(std::move(Root)));
}

} // namespace tanks
