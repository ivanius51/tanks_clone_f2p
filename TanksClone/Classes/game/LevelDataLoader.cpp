#include "game/LevelDataLoader.h"

#include <string>

#include "cocos2d.h"

#include "model/LevelJsonWriter.h"
#include "model/Log.h"
#include "templates/TContainers.h"

namespace game
{

namespace
{
	constexpr float DefaultWaveDelay = 1.0f;

	model::JsonValue Number(const float InValue)
	{
		return model::LevelJsonWriter::FromFloat(InValue);
	}

	model::JsonValue Strings(const TArray<std::string>& InItems)
	{
		model::JsonValue::Array Items;
		for ( const std::string& Item : InItems )
		{
			Items.EmplaceBack(Item);
		}
		return model::JsonValue(std::move(Items));
	}

	model::JsonValue Strings(const TArray<FPersistentString>& InItems)
	{
		model::JsonValue::Array Items;
		for ( const FPersistentString Item : InItems )
		{
			Items.EmplaceBack(std::string(Item.View()));
		}
		return model::JsonValue(std::move(Items));
	}

	TArray<FPersistentString> ReadIds(const model::IConfigNode& InNode, const std::string_view InKey)
	{
		TArray<std::string> Texts;
		InNode.TryRead(InKey, Texts);
		TArray<FPersistentString> Ids;
		Ids.Reserve(Texts.Size());
		for ( const std::string& Text : Texts )
		{
			Ids.PushBack(FPersistentString{ Text });
		}
		return Ids;
	}

	model::JsonValue TypeNames(const TArray<EEnemyType>& InTypes)
	{
		model::JsonValue::Array Items;
		for ( const EEnemyType Type : InTypes )
		{
			Items.EmplaceBack(EnemyTypeName(Type));
		}
		return model::JsonValue(std::move(Items));
	}

	model::JsonValue RuleToJson(const SpawnRule& InRule)
	{
		model::JsonValue::Object Members;
		if ( !InRule.Types.IsEmpty() )
		{
			Members.InsertOrAssign("types", TypeNames(InRule.Types));
		}
		if ( !InRule.Enemies.IsEmpty() )
		{
			Members.InsertOrAssign("enemies", Strings(InRule.Enemies));
		}
		if ( !InRule.ExcludeTypes.IsEmpty() )
		{
			Members.InsertOrAssign("exclude_types", TypeNames(InRule.ExcludeTypes));
		}
		if ( !InRule.ExcludeEnemies.IsEmpty() )
		{
			Members.InsertOrAssign("exclude", Strings(InRule.ExcludeEnemies));
		}
		return model::JsonValue(std::move(Members));
	}

	model::JsonValue WaveToJson(const Wave& InWave)
	{
		model::JsonValue::Array Spawns;
		for ( const WaveEntry& Entry : InWave.Entries )
		{
			model::JsonValue::Object Item;
			Item.InsertOrAssign("spawner", model::JsonValue(std::string(1, Entry.Spawner)));
			Item.InsertOrAssign("count", model::JsonValue(Entry.Count));
			Spawns.EmplaceBack(std::move(Item));
		}
		model::JsonValue::Object Members;
		Members.InsertOrAssign("delay", Number(InWave.Delay));
		Members.InsertOrAssign("max_alive", model::JsonValue(InWave.MaxAlive));
		Members.InsertOrAssign("interval", Number(InWave.Interval));
		Members.InsertOrAssign("spawns", model::JsonValue(std::move(Spawns)));
		return model::JsonValue(std::move(Members));
	}

	bool SameWave(const Wave& InFirst, const Wave& InSecond)
	{
		if ( InFirst.Delay != InSecond.Delay || InFirst.MaxAlive != InSecond.MaxAlive || InFirst.Interval != InSecond.Interval
			|| InFirst.Entries.Size() != InSecond.Entries.Size() )
		{
			return false;
		}
		for ( size_t Index = 0; Index < InFirst.Entries.Size(); ++Index )
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
	SimpleWave.Entries.PushBack({ tiles::EnemySpawn, InLevel.TotalEnemies });
	return SimpleWave;
}

bool LevelDataLoader::HasAuthoredWaves(const LevelData& InLevel)
{
	if ( InLevel.Waves.IsEmpty() )
	{
		return false;
	}
	return !(InLevel.Waves.Size() == 1 && SameWave(InLevel.Waves.Front(), MakeSimpleWave(InLevel)));
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
		CurrentLevel.EnemyIds = ReadIds(*Enemies, "types");
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
			CurrentLevel.Waves.PushBack(ReadWave(*Waves->GetItem(Index)));
		}
	}
	if ( CurrentLevel.Waves.IsEmpty() )
	{
		CurrentLevel.Waves.PushBack(MakeSimpleWave(CurrentLevel));
	}

	if ( const model::IConfigNode* const Fog = InNode.FindChild("fog_of_war") )
	{
		CurrentLevel.Fog.bEnabled = Fog->GetValueOr("enabled", CurrentLevel.Fog.bEnabled);
		CurrentLevel.Fog.VisionTiles = Fog->GetValueOr("vision_tiles", CurrentLevel.Fog.VisionTiles);
		CurrentLevel.Fog.bShowInitialWalls = Fog->GetValueOr("show_initial_walls", CurrentLevel.Fog.bShowInitialWalls);
	}

	InNode.TryRead("ground", CurrentLevel.Ground);
	CurrentLevel.Layers = model::LevelLayers::FromConfig(InNode);

	model::JsonValue::Object Unknown;
	for ( const std::string& Key : InNode.GetKeys() )
	{
		if ( !model::LevelJsonWriter::IsKnownKey(Key) )
		{
			Unknown.InsertOrAssign(Key, model::LevelJsonWriter::FromNode(*InNode.FindChild(Key)));
		}
	}
	if ( !Unknown.IsEmpty() )
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
	Rule.Enemies = ReadIds(InNode, "enemies");
	Rule.ExcludeTypes = ReadEnemyTypes(InNode, "exclude_types");
	Rule.ExcludeEnemies = ReadIds(InNode, "exclude");
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
			CurrentWave.Entries.PushBack(Entry);
		}
	}
	return CurrentWave;
}

TArray<EEnemyType> LevelDataLoader::ReadEnemyTypes(const model::IConfigNode& InNode, const std::string_view InKey)
{
	TArray<std::string> Names;
	InNode.TryRead(InKey, Names);

	TArray<EEnemyType> Types;
	for ( const std::string& Name : Names )
	{
		EEnemyType Type = EEnemyType::Medium;
		if ( EnumFromName(std::string_view(Name), Type) )
		{
			Types.PushBack(Type);
		}
		else
		{
			model::Log::Error("LevelLoader: unknown enemy class %s", Name.c_str());
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
	Root.InsertOrAssign("name", model::JsonValue(InLevel.Name));
	if ( !InLevel.Music.empty() )
	{
		Root.InsertOrAssign("music", model::JsonValue(InLevel.Music));
	}
	if ( !InLevel.Tileset.empty() )
	{
		Root.InsertOrAssign("tileset", model::JsonValue(InLevel.Tileset));
	}
	if ( !(InLevel.Fog == FogOfWar()) )
	{
		model::JsonValue::Object Fog;
		Fog.InsertOrAssign("enabled", model::JsonValue(InLevel.Fog.bEnabled));
		Fog.InsertOrAssign("vision_tiles", Number(InLevel.Fog.VisionTiles));
		Fog.InsertOrAssign("show_initial_walls", model::JsonValue(InLevel.Fog.bShowInitialWalls));
		Root.InsertOrAssign("fog_of_war", model::JsonValue(std::move(Fog)));
	}
	Root.InsertOrAssign("rows", Strings(InLevel.Rows));
	if ( !InLevel.Ground.IsEmpty() )
	{
		Root.InsertOrAssign("ground", Strings(InLevel.Ground));
	}

	if ( !HasAuthoredWaves(InLevel) || !InLevel.EnemyIds.IsEmpty() )
	{
		model::JsonValue::Object Enemies;
		Enemies.InsertOrAssign("total", model::JsonValue(InLevel.TotalEnemies));
		Enemies.InsertOrAssign("max_alive", model::JsonValue(InLevel.MaxAlive));
		Enemies.InsertOrAssign("spawn_interval", Number(InLevel.SpawnInterval));
		Enemies.InsertOrAssign("types", Strings(InLevel.EnemyIds));
		Root.InsertOrAssign("enemies", model::JsonValue(std::move(Enemies)));
	}

	if ( !InLevel.Spawners.IsEmpty() )
	{
		model::JsonValue::Object Spawners;
		for ( const auto& Spawner : InLevel.Spawners )
		{
			Spawners.InsertOrAssign(std::string(1, Spawner.Key), RuleToJson(Spawner.Value));
		}
		Root.InsertOrAssign("spawners", model::JsonValue(std::move(Spawners)));
	}
	if ( HasAuthoredWaves(InLevel) )
	{
		model::JsonValue::Array Waves;
		for ( const Wave& CurrentWave : InLevel.Waves )
		{
			Waves.PushBack(WaveToJson(CurrentWave));
		}
		Root.InsertOrAssign("waves", model::JsonValue(std::move(Waves)));
	}
	if ( !InLevel.Layers.GetObjects().IsEmpty() )
	{
		Root.InsertOrAssign("objects", InLevel.Layers.ObjectsToJson());
	}
	if ( !InLevel.Layers.GetPoints().IsEmpty() )
	{
		Root.InsertOrAssign("points", InLevel.Layers.PointsToJson());
	}
	if ( !InLevel.AiFeatures.IsEmpty() )
	{
		model::JsonValue::Object Switches;
		for ( const auto& Switch : InLevel.AiFeatures )
		{
			Switches.InsertOrAssign(Switch.Key.View(), model::JsonValue(Switch.Value));
		}
		Root.InsertOrAssign("ai_features", model::JsonValue(std::move(Switches)));
	}
	if ( !InLevel.Script.IsEmpty() || !InLevel.Script.Actors.IsEmpty() )
	{
		Root.InsertOrAssign("script", InLevel.Script.ToJson());
	}
	return model::LevelJsonWriter::Write(model::JsonValue(std::move(Root)));
}

} // namespace game
