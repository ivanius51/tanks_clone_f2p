#include "tests/UnitTests.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "2d/CCFontAtlas.h"
#include "2d/CCFontAtlasCache.h"
#include "ui/CocosConvert.h"

#include "core/ConfigReader.h"
#include "core/DataConverter.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/JsonConfigNode.h"
#include "core/ManagerSet.h"
#include "core/ObjectCast.h"
#include "core/ResourceIds.h"
#include "core/TypeList.h"
#include "game/ai/AiQueries.h"
#include "game/ai/AiSettings.h"
#include "game/ai/AiSettingsLoader.h"
#include "game/components/Ability.h"
#include "game/components/Buff.h"
#include "game/components/Stats.h"
#include "game/components/StatsLoader.h"
#include "game/EnemyCatalog.h"
#include "game/EnemyCatalogLoader.h"
#include "game/EnemySpawner.h"
#include "game/LevelDataLoader.h"
#include "game/LevelLoader.h"
#include "game/objects/Tank.h"
#include "game/objects/TankLoader.h"
#include "game/objects/Wall.h"
#include "game/ObjectsFactory.h"
#include "game/ResourceManifest.h"
#include "game/PlayerControls.h"
#include "model/InputBindings.h"
#include "model/InputContextStack.h"
#include "platform/cocos/CocosKeyboardInput.h"
#include "platform/cocos/LegacyProfile.h"
#include "platform/SplashWindow.h"
#include "ports/IAudioOutput.h"
#include "ports/IStorage.h"
#include "services/ProfileSerializer.h"
#include "model/EditorLayout.h"
#include "model/EditorPalette.h"
#include "model/JsonValue.h"
#include "model/MathTypes.h"
#include "model/Random.h"
#include "model/NodeNavigator.h"
#include "model/TurnState.h"
#include "model/UpgradeTree.h"
#include "model/RefCounted.h"
#include "model/ResourceStatus.h"
#include "model/ResourceTable.h"
#include "model/ScreenLayout.h"
#include "model/TextSize.h"
#include "model/WallMask.h"
#include "model/WorkQueue.h"
#include "scenes/MenuScene.h"
#include "scenes/SettingsScene.h"
#include "services/ResourceHolder.h"
#include "services/ResourceLoadJob.h"
#include "services/AudioManager.h"
#include "services/DisplaySettings.h"
#include "services/ResourceManager.h"
#include "services/ProfileManager.h"
#include "services/UiStyle.h"
#include "services/UpgradeManager.h"
#include "tests/Showcase.h"
#include "tests/TestSelection.h"
#include "ui/Widgets.h"

namespace tanks
{
namespace tests
{

namespace
{
	std::string PathOf(const model::ResourceId& InId, const model::ResourceKind InKind)
	{
		return GameContext::Get().GetManager<ResourceManager>().ResolvePath(InId, InKind);
	}

	std::string PlayerTankPath()
	{
		return PathOf(ids::configs::PlayerTank, model::ResourceKind::Config);
	}

	/** A small data type that reads itself, for the IConfigNode tests. */
	struct TestWaveDef
	{
		int Count = 0;
		float Delay = 0.0f;

		bool FromConfig(const model::IConfigNode& InNode)
		{
			Count = InNode.GetInt("count");
			Delay = InNode.GetValueOr("delay", 0.0f);
			return true;
		}
	};

	Tank* CreateTank(TestContext& InContext, const std::string& InConfig)
	{
		auto CurrentTank = GameContext::Get().GetManager<ObjectsFactory>().CreateFromConfig<Tank>(InConfig, InContext.GetSandbox());
		InContext.Expect(CurrentTank != nullptr, "tank " + InConfig + " is created");
		return CurrentTank;
	}

	BuffDef MakeBuff(std::string_view InId, const float InDuration)
	{
		BuffDef CurrentDef;
		CurrentDef.Id = InId;
		CurrentDef.Duration = InDuration;
		return CurrentDef;
	}

	LevelData ValidLevel()
	{
		LevelData CurrentLevel = LevelLoader::CreateEmpty();
		CurrentLevel.Rows[GridRows - 1][2] = tiles::PlayerSpawn;
		CurrentLevel.Rows[0][0] = tiles::EnemySpawn;
		return CurrentLevel;
	}

	void AddStatsTests(TestSuite& InSuite)
	{
		InSuite.Add("Stats: modifiers add first, then multiply", [](TestContext& InContext)
		{
			StatSet CurrentStats;
			CurrentStats.SetBase(StatType::Damage, 2.0f);
			CurrentStats.ApplyModifiers({ { StatType::Damage, 1.0f, 1.0f }, { StatType::Damage, 0.0f, 2.0f } });
			InContext.ExpectNear(CurrentStats.Get(StatType::Damage), 6.0f, 0.001f, "(2 + 1) * 2");
			InContext.ExpectNear(CurrentStats.GetBase(StatType::Damage), 2.0f, 0.001f, "the base value is kept");
		});

		InSuite.Add("Stats: several multipliers stack", [](TestContext& InContext)
		{
			StatSet CurrentStats;
			CurrentStats.SetBase(StatType::ReloadTime, 0.5f);
			CurrentStats.ApplyModifiers({ { StatType::ReloadTime, 0.0f, 0.5f }, { StatType::ReloadTime, 0.0f, 0.5f } });
			InContext.ExpectNear(CurrentStats.Get(StatType::ReloadTime), 0.125f, 0.001f, "0.5 * 0.5 * 0.5");
			CurrentStats.ApplyModifiers({});
			InContext.ExpectNear(CurrentStats.Get(StatType::ReloadTime), 0.5f, 0.001f, "no modifiers gives the base value again");
		});

		InSuite.Add("Stats: modifiers of other stats do not interfere", [](TestContext& InContext)
		{
			StatSet CurrentStats;
			CurrentStats.SetBase(StatType::Speed, 100.0f);
			CurrentStats.SetBase(StatType::Armor, 1.0f);
			CurrentStats.ApplyModifiers({ { StatType::Speed, 0.0f, 1.5f } });
			InContext.ExpectNear(CurrentStats.Get(StatType::Speed), 150.0f, 0.001f, "speed changed");
			InContext.ExpectNear(CurrentStats.Get(StatType::Armor), 1.0f, 0.001f, "armor untouched");
		});

		InSuite.Add("Stats: modifiers are read from config values", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(
				R"([{"stat":"Speed","mul":1.5},{"stat":"NoSuchStat"},{"stat":"MaxAmmo","add":5},{"add":2}])", "test");
			const auto CurrentModifiers = StatsLoader::ReadModifiers(*Root);
			InContext.ExpectEqual(CurrentModifiers.size(), static_cast<size_t>(2), "unknown and missing stats are skipped");
			if ( CurrentModifiers.size() == 2 )
			{
				InContext.Expect(CurrentModifiers[0].Stat == StatType::Speed, "first is Speed");
				InContext.ExpectNear(CurrentModifiers[0].Mul, 1.5f, 0.001f, "mul read");
				InContext.ExpectNear(CurrentModifiers[0].Add, 0.0f, 0.001f, "add defaults to 0");
				InContext.Expect(CurrentModifiers[1].Stat == StatType::MaxAmmo, "second is MaxAmmo");
				InContext.ExpectNear(CurrentModifiers[1].Add, 5.0f, 0.001f, "add read");
				InContext.ExpectNear(CurrentModifiers[1].Mul, 1.0f, 0.001f, "mul defaults to 1");
			}
		});

		InSuite.Add("Stats: stat names map to stat types", [](TestContext& InContext)
		{
			StatType Stat = StatType::Damage;
			InContext.Expect(EnumFromName("ReloadTime", Stat) && Stat == StatType::ReloadTime, "known name");
			InContext.Expect(!EnumFromName("reloadtime", Stat) && Stat == StatType::ReloadTime, "names are case sensitive, value untouched");
			const auto NotArray = JsonConfigNode::Parse(R"({"stat":"Speed"})", "test");
			InContext.Expect(StatsLoader::ReadModifiers(*NotArray).empty(), "an object is not a modifier list");
		});
	}

	void AddBuffTests(TestSuite& InSuite)
	{
		InSuite.Add("Buffs: a timed buff runs out", [](TestContext& InContext)
		{
			BuffSet CurrentBuffs;
			CurrentBuffs.Add(MakeBuff("a", 1.0f));
			InContext.Expect(!CurrentBuffs.Update(0.5f), "still active after 0.5 s");
			InContext.ExpectEqual(CurrentBuffs.GetActive().size(), static_cast<size_t>(1), "one active buff");
			InContext.Expect(CurrentBuffs.Update(0.6f), "reports that a buff ran out");
			InContext.Expect(CurrentBuffs.GetActive().empty(), "nothing active afterwards");
		});

		InSuite.Add("Buffs: adding the same buff again restarts its timer", [](TestContext& InContext)
		{
			BuffSet CurrentBuffs;
			CurrentBuffs.Add(MakeBuff("a", 1.0f));
			CurrentBuffs.Update(0.8f);
			CurrentBuffs.Add(MakeBuff("a", 1.0f));
			InContext.ExpectEqual(CurrentBuffs.GetActive().size(), static_cast<size_t>(1), "still one buff");
			InContext.Expect(!CurrentBuffs.Update(0.5f), "alive: the timer was restarted");
		});

		InSuite.Add("Buffs: a permanent buff never runs out", [](TestContext& InContext)
		{
			BuffSet CurrentBuffs;
			CurrentBuffs.Add(MakeBuff("perm", -1.0f));
			InContext.Expect(!CurrentBuffs.Update(1000.0f), "nothing expires");
			InContext.ExpectEqual(CurrentBuffs.GetActive().size(), static_cast<size_t>(1), "still active");
		});

		InSuite.Add("Buffs: invulnerability and modifiers are reported", [](TestContext& InContext)
		{
			BuffSet CurrentBuffs;
			InContext.Expect(!CurrentBuffs.IsInvulnerable(), "not invulnerable at first");

			BuffDef Shield = MakeBuff("shield", 2.0f);
			Shield.bInvulnerable = true;
			BuffDef Fast = MakeBuff("fast", 2.0f);
			Fast.Modifiers.push_back({ StatType::Speed, 0.0f, 2.0f });
			CurrentBuffs.Add(Shield);
			CurrentBuffs.Add(Fast);

			InContext.Expect(CurrentBuffs.IsInvulnerable(), "shield makes invulnerable");
			InContext.ExpectEqual(CurrentBuffs.CollectModifiers().size(), static_cast<size_t>(1), "one modifier collected");
			CurrentBuffs.Remove("shield");
			InContext.Expect(!CurrentBuffs.IsInvulnerable(), "removing the shield ends it");
		});
	}

	void AddDataTests(TestSuite& InSuite)
	{
		InSuite.Add("Enums: names and unknown names are parsed", [](TestContext& InContext)
		{
			EnemyType Kind = EnemyType::Light;
			InContext.Expect(EnumFromName("Heavy", Kind) && Kind == EnemyType::Heavy, "Heavy is a known class");
			InContext.Expect(!EnumFromName("Nonsense", Kind) && Kind == EnemyType::Heavy, "an unknown class is reported and changes nothing");
			AiType Ai = AiType::None;
			InContext.Expect(EnumFromName("Hunter", Ai) && Ai == AiType::Hunter, "ai type");
			InContext.Expect(!EnumFromName("None", Ai), "None is not a configurable ai");
			InContext.Expect(ParseTeam(cocos2d::Value("Enemy")) == Team::Enemy, "team");
			InContext.Expect(ParseObjectType(cocos2d::Value("Wall")) == ObjectType::Wall, "object type");
			InContext.Expect(ParseBonusType(cocos2d::Value("Buff")) == BonusType::Buff, "bonus type");
		});

		InSuite.Add("Enemies: an enemy def is read from its config", [](TestContext& InContext)
		{
			const auto Full = JsonConfigNode::Parse(R"({"name":"enemy_sniper","atlas":"textures/enemies/Sniper.plist","enemy_type":"Heavy","health":3})", "test");
			const EnemyDef Sniper = EnemyCatalogLoader::ReadEnemy(*Full, "configs/enemies/Sniper.json");
			InContext.ExpectEqual(Sniper.Id, std::string("Sniper"), "id is the file name");
			InContext.ExpectEqual(Sniper.ConfigId, std::string("configs/enemies/Sniper.json"), "config id kept");
			InContext.ExpectEqual(Sniper.NameKey, std::string("enemy_sniper"), "name read");
			InContext.ExpectEqual(Sniper.Atlas, std::string("textures/enemies/Sniper.plist"), "atlas read");
			InContext.Expect(Sniper.Kind == EnemyType::Heavy, "class read by name");

			const auto Bare = JsonConfigNode::Parse(R"({"enemy_type":"Nonsense"})", "test");
			const EnemyDef Plain = EnemyCatalogLoader::ReadEnemy(*Bare, "configs/enemies/Plain.json");
			InContext.ExpectEqual(Plain.NameKey, std::string("Plain"), "name defaults to the id");
			InContext.Expect(Plain.Atlas.empty(), "no atlas");
			InContext.Expect(Plain.Kind == EnemyType::Medium, "unknown class keeps the default");
		});

		InSuite.Add("Tanks: a tank def is read from its config", [](TestContext& InContext)
		{
			const auto Full = JsonConfigNode::Parse(R"({
				"armor": 2, "reload_time": 0.8, "max_ammo": -1, "score": 300, "coins": 25,
				"enemy_type": "Boss", "ai": "Hunter", "buffs": ["regen"],
				"weapons": [{"id": "mg", "name": "WEAPON_MG", "icon": "icon_mg", "bullet": "configs/bullet_mg.json", "reload_time": 0.1, "damage": 2, "uses_ammo": false}],
				"abilities": [{"type": "Shoot"}, {"type": "Buff", "buff": "shield", "cooldown": 8}]
			})", "test");
			const TankDef Boss = TankLoader::ReadTank(*Full);
			InContext.ExpectNear(Boss.Armor, 2.0f, 0.001f, "armor");
			InContext.ExpectNear(Boss.ReloadTime, 0.8f, 0.001f, "reload time");
			InContext.ExpectEqual(Boss.MaxAmmo, -1, "unlimited ammo");
			InContext.ExpectEqual(Boss.ScoreReward, 300, "score");
			InContext.ExpectEqual(Boss.CoinReward, 25, "coins");
			InContext.Expect(Boss.EnemyKind == EnemyType::Boss, "enemy class");
			InContext.Expect(Boss.Ai == AiType::Hunter, "ai");
			InContext.Expect(Boss.PermanentBuffs == std::vector<std::string>{ "regen" }, "permanent buffs");
			InContext.Expect(Boss.Weapons.size() == 1 && Boss.Weapons[0].Id == "mg" && Boss.Weapons[0].BulletConfig == "configs/bullet_mg.json"
				&& Boss.Weapons[0].Damage == 2 && !Boss.Weapons[0].bUsesAmmo, "weapon read");
			InContext.Expect(Boss.Abilities.size() == 2 && Boss.Abilities[1].Type == "Buff" && Boss.Abilities[1].BuffId == "shield", "abilities read");
			InContext.ExpectNear(Boss.Abilities[1].Cooldown, 8.0f, 0.001f, "ability cooldown");

			const auto Bare = JsonConfigNode::Parse(R"({"ai": "Nonsense"})", "test");
			const TankDef Plain = TankLoader::ReadTank(*Bare);
			InContext.ExpectNear(Plain.ReloadTime, TankDef().ReloadTime, 0.001f, "reload time default");
			InContext.ExpectEqual(Plain.MaxAmmo, TankDef().MaxAmmo, "ammo default");
			InContext.Expect(Plain.Ai == AiType::None, "unknown ai means no ai");
			InContext.Expect(Plain.Weapons.size() == 1 && Plain.Weapons[0].BulletConfig == PathOf(ids::configs::Bullet, model::ResourceKind::Config), "one default cannon");
			InContext.Expect(Plain.Abilities.size() == 1 && Plain.Abilities[0].Type == "Shoot", "the tank just shoots");
		});

		InSuite.Add("Levels: a level is read from its config", [](TestContext& InContext)
		{
			const auto Simple = JsonConfigNode::Parse(R"({
				"name": "L1", "tileset": "desert", "rows": ["P.E", "..."],
				"enemies": {"total": 4, "max_alive": 2, "spawn_interval": 1.5, "types": ["Basic", "Fast"]}
			})", "test");
			LevelData Level;
			InContext.Expect(LevelDataLoader::ReadLevel(*Simple, Level), "simple level read");
			InContext.ExpectEqual(Level.Name, std::string("L1"), "name");
			InContext.ExpectEqual(Level.Tileset, std::string("desert"), "tileset");
			InContext.Expect(Level.Rows.size() == 2 && Level.Rows[0] == "P.E", "rows");
			InContext.Expect(Level.EnemyIds == std::vector<std::string>{ "Basic", "Fast" }, "enemy ids");
			InContext.Expect(Level.Waves.size() == 1 && Level.Waves[0].MaxAlive == 2 && Level.Waves[0].Entries.size() == 1
				&& Level.Waves[0].Entries[0].Count == 4 && Level.Waves[0].Entries[0].Spawner == tiles::EnemySpawn, "one default wave from the simple form");

			const auto Advanced = JsonConfigNode::Parse(R"({
				"rows": ["P1"],
				"spawners": {"1": {"types": ["Heavy", "Nonsense"], "exclude": ["Boss"]}, "too_long": {}},
				"waves": [{"delay": 4, "max_alive": 5, "interval": 0.5, "spawns": [{"spawner": "1", "count": 3}, {}]}]
			})", "test");
			InContext.Expect(LevelDataLoader::ReadLevel(*Advanced, Level), "advanced level read");
			InContext.Expect(Level.Spawners.size() == 1 && Level.Spawners.count('1') == 1, "only one-letter spawners");
			const SpawnRule& Rule = Level.Spawners['1'];
			InContext.Expect(Rule.Types == std::vector<EnemyType>{ EnemyType::Heavy }, "unknown classes skipped");
			InContext.Expect(Rule.ExcludeEnemies == std::vector<std::string>{ "Boss" }, "exclusions");
			InContext.Expect(Level.Waves.size() == 1 && Level.Waves[0].Entries.size() == 2, "one wave, two entries");
			InContext.ExpectNear(Level.Waves[0].Delay, 4.0f, 0.001f, "wave delay");
			InContext.Expect(Level.Waves[0].Entries[0].Spawner == '1' && Level.Waves[0].Entries[0].Count == 3, "spawn entry");
			InContext.Expect(Level.Waves[0].Entries[1].Spawner == tiles::EnemySpawn && Level.Waves[0].Entries[1].Count == 1, "spawn entry defaults");

			const auto NoRows = JsonConfigNode::Parse(R"({"name": "broken"})", "test");
			InContext.Expect(!LevelDataLoader::ReadLevel(*NoRows, Level), "a level without rows is rejected");
		});

		InSuite.Add("Enemies: every enemy config gives a catalog entry", [](TestContext& InContext)
		{
			const auto& All = GameContext::Get().GetManager<EnemyCatalog>().GetAll();
			InContext.Expect(!All.empty(), "the catalog is not empty");
			for ( const EnemyDef& Def : All )
			{
				InContext.Expect(!Def.Id.empty() && Def.ConfigId.find(Def.Id) != std::string::npos, "id comes from the file name: " + Def.ConfigId);
			}
		});

		InSuite.Add("Levels: the validator accepts a good level and rejects bad ones", [](TestContext& InContext)
		{
			InContext.Expect(LevelLoader::Validate(ValidLevel()).empty(), "a level with P and E is valid");

			LevelData NoPlayer = ValidLevel();
			NoPlayer.Rows[GridRows - 1][2] = tiles::Empty;
			InContext.ExpectEqual(LevelLoader::Validate(NoPlayer), std::string("LEVEL_ERROR_PLAYER"), "no player");

			LevelData NoEnemies = ValidLevel();
			NoEnemies.Rows[0][0] = tiles::Empty;
			InContext.ExpectEqual(LevelLoader::Validate(NoEnemies), std::string("LEVEL_ERROR_ENEMY"), "no spawner");

			LevelData BadTile = ValidLevel();
			BadTile.Rows[3][3] = 'x';
			InContext.ExpectEqual(LevelLoader::Validate(BadTile), std::string("LEVEL_ERROR_TILE"), "unknown tile");

			LevelData BadSize = ValidLevel();
			BadSize.Rows[2] = "...";
			InContext.ExpectEqual(LevelLoader::Validate(BadSize), std::string("LEVEL_ERROR_SIZE"), "wrong row width");

			LevelData TwoPlayers = ValidLevel();
			TwoPlayers.Rows[5][5] = tiles::PlayerSpawn;
			InContext.ExpectEqual(LevelLoader::Validate(TwoPlayers), std::string("LEVEL_ERROR_PLAYER"), "two players");
		});

		InSuite.Add("Levels: a wave may only use spawners that are on the map", [](TestContext& InContext)
		{
			LevelData CurrentLevel = ValidLevel();
			Wave CurrentWave;
			CurrentWave.Entries.push_back({ '3', 2 });
			CurrentLevel.Waves.push_back(CurrentWave);
			InContext.ExpectEqual(LevelLoader::Validate(CurrentLevel), std::string("LEVEL_ERROR_ENEMY"), "spawner 3 is not on the map");

			CurrentLevel.Rows[1][1] = '3';
			InContext.Expect(LevelLoader::Validate(CurrentLevel).empty(), "valid once the spawner 3 is placed");
			InContext.ExpectEqual(LevelLoader::GetTotalEnemies(CurrentLevel), 2, "total enemies of the waves");
		});

		InSuite.Add("Levels: spawner pools follow classes, names and restrictions", [](TestContext& InContext)
		{
			const auto& Catalog = GameContext::Get().GetManager<EnemyCatalog>();
			InContext.Expect(Catalog.GetAll().size() >= 3, "the catalog has enemies");

			SpawnRule Anyone;
			InContext.ExpectEqual(LevelLoader::ResolvePool(Anyone).size(), Catalog.GetAll().size(), "an empty rule allows everyone");

			SpawnRule LightOnly;
			LightOnly.Types = { EnemyType::Light };
			const auto Light = LevelLoader::ResolvePool(LightOnly);
			InContext.ExpectEqual(Light.size(), Catalog.GetByType(EnemyType::Light).size(), "only the Light class");
			for ( const auto& CurrentId : Light )
			{
				InContext.Expect(Catalog.Find(CurrentId) && Catalog.Find(CurrentId)->Kind == EnemyType::Light, CurrentId + " is Light");
			}

			SpawnRule BossOnly;
			BossOnly.Enemies = { "Boss" };
			const auto Boss = LevelLoader::ResolvePool(BossOnly);
			InContext.Expect(Boss.size() == 1 && Boss[0] == "Boss", "a particular enemy");

			SpawnRule Restricted;
			Restricted.Types = { EnemyType::Light, EnemyType::Medium };
			Restricted.ExcludeEnemies = { "Basic" };
			const auto Pool = LevelLoader::ResolvePool(Restricted);
			InContext.Expect(std::find(Pool.begin(), Pool.end(), "Basic") == Pool.end(), "an excluded enemy is not sent");
			InContext.Expect(std::find(Pool.begin(), Pool.end(), "Fast") != Pool.end(), "other enemies of the class stay");

			SpawnRule NoHeavy;
			NoHeavy.ExcludeTypes = { EnemyType::Heavy, EnemyType::Boss };
			for ( const auto& CurrentId : LevelLoader::ResolvePool(NoHeavy) )
			{
				const EnemyType CurrentType = Catalog.Find(CurrentId)->Kind;
				InContext.Expect(CurrentType != EnemyType::Heavy && CurrentType != EnemyType::Boss, CurrentId + " is not an excluded class");
			}
		});

		InSuite.Add("Levels: the editor format lists the level and its enemies", [](TestContext& InContext)
		{
			LevelData CurrentLevel = ValidLevel();
			CurrentLevel.TotalEnemies = 9;
			const std::string Json = LevelLoader::ToJson(CurrentLevel);
			InContext.Expect(Json.find("\"total\": 9") != std::string::npos, "total enemies written");
			InContext.Expect(Json.find("\"Basic\"") != std::string::npos, "enemy ids written");
			InContext.Expect(Json.find('P') != std::string::npos, "the map is written");
		});

		InSuite.Add("Data: every built-in level loads and every spawner can send someone", [](TestContext& InContext)
		{
			const int CurrentCount = LevelLoader::GetBuiltinCount();
			InContext.Expect(CurrentCount >= 1, "there are built-in levels");
			for ( int Index = 0; Index < CurrentCount; ++Index )
			{
				LevelData CurrentLevel;
				const std::string CurrentId = LevelLoader::BuiltinId(Index);
				InContext.Expect(LevelLoader::Load(CurrentId, CurrentLevel), CurrentId + " loads");
				for ( const auto& CurrentWave : CurrentLevel.Waves )
				{
					for ( const auto& CurrentEntry : CurrentWave.Entries )
					{
						const auto Pool = LevelLoader::ResolvePool(LevelLoader::GetRule(CurrentLevel, CurrentEntry.Spawner));
						InContext.Expect(!Pool.empty(), CurrentId + ": spawner " + std::string(1, CurrentEntry.Spawner) + " has a non-empty pool");
					}
				}
			}
		});

		InSuite.Add("Data: every enemy config is complete and consistent", [](TestContext& InContext)
		{
			for ( const auto& Enemy : GameContext::Get().GetManager<EnemyCatalog>().GetAll() )
			{
				const cocos2d::ValueMap& ConfigData = GameContext::Get().GetManager<ConfigReader>().GetObjectData(Enemy.ConfigId).Values;
				InContext.Expect(DataConverter::GetInt(ConfigData, "health", 0) > 0, Enemy.Id + ": health > 0");
				InContext.Expect(DataConverter::GetFloat(ConfigData, "speed", 0.0f) > 0.0f, Enemy.Id + ": speed > 0");
				InContext.Expect(DataConverter::GetInt(ConfigData, "score", 0) > 0, Enemy.Id + ": has a score reward");
				InContext.Expect(ConfigData.find("enemy_type") != ConfigData.end(), Enemy.Id + ": has an enemy_type");
				InContext.Expect(TankLoader::ReadTank(GameContext::Get().GetManager<ConfigReader>().GetConfig(Enemy.ConfigId)).Ai != AiType::None, Enemy.Id + ": has an ai");

				auto AbilitiesIt = ConfigData.find("abilities");
				InContext.Expect(AbilitiesIt != ConfigData.end() && AbilitiesIt->second.getType() == cocos2d::Value::Type::VECTOR, Enemy.Id + ": has abilities");
				if ( AbilitiesIt == ConfigData.end() || AbilitiesIt->second.getType() != cocos2d::Value::Type::VECTOR )
				{
					continue;
				}
				for ( const auto& Item : AbilitiesIt->second.asValueVector() )
				{
					const cocos2d::ValueMap& CurrentEntry = Item.asValueMap();
					InContext.Expect(CreateAbility(DataConverter::GetString(CurrentEntry, "type", "")) != nullptr, Enemy.Id + ": ability type is known");
					const std::string Buff = DataConverter::GetString(CurrentEntry, "buff", "");
					InContext.Expect(Buff.empty() || GameContext::Get().GetManager<BuffCatalog>().Find(Buff) != nullptr, Enemy.Id + ": buff " + Buff + " exists");
				}
			}
		});

		InSuite.Add("Data: upgrades are complete and affordable in order", [](TestContext& InContext)
		{
			InContext.Expect(!GameContext::Get().GetManager<UpgradeManager>().GetUpgrades().empty(), "there are upgrades");
			for ( const auto& Upgrade : GameContext::Get().GetManager<UpgradeManager>().GetUpgrades() )
			{
				InContext.Expect(!Upgrade.Modifiers.empty() || !Upgrade.AutoFireWeapon.empty() || !Upgrade.UnlockWeapon.empty() || !Upgrade.Weapon.empty(),
					Upgrade.Id + ": has an effect");
				InContext.Expect(Upgrade.Requires.empty() || GameContext::Get().GetManager<UpgradeManager>().GetRequirement(Upgrade.Id),
					Upgrade.Id + ": its requirement exists");
				InContext.Expect(Upgrade.MaxLevel > 0, Upgrade.Id + ": has levels");
				InContext.Expect(Upgrade.BaseCost > 0, Upgrade.Id + ": costs something");
				InContext.ExpectEqual(GameContext::Get().GetManager<UpgradeManager>().GetNextCost(Upgrade.Id), Upgrade.BaseCost, Upgrade.Id + ": first level costs the base price");
			}
		});
	}

	void AddTankTests(TestSuite& InSuite)
	{
		InSuite.Add("Tank: armor reduces damage but a hit always hurts", [](TestContext& InContext)
		{
			Tank* Heavy = CreateTank(InContext, "configs/enemies/Heavy.json"); // armor 1
			if ( !Heavy )
			{
				return;
			}
			const int Before = Heavy->GetHealth();
			Heavy->ApplyDamage(4, Team::Player);
			InContext.ExpectEqual(Heavy->GetHealth(), Before - 3, "4 damage - 1 armor");
			Heavy->ApplyDamage(1, Team::Player);
			InContext.ExpectEqual(Heavy->GetHealth(), Before - 4, "a weak hit still does 1");
		});

		InSuite.Add("Tank: a shield absorbs everything", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateTank(InContext, PlayerTankPath());
			if ( !CurrentTank )
			{
				return;
			}
			CurrentTank->GetBuffs().Add(*GameContext::Get().GetManager<BuffCatalog>().Find("shield"));
			CurrentTank->RefreshStats();
			const int Before = CurrentTank->GetHealth();
			CurrentTank->ApplyDamage(100, Team::Enemy);
			InContext.ExpectEqual(CurrentTank->GetHealth(), Before, "no damage under the shield");
			InContext.Expect(!CurrentTank->IsDestroying(), "still alive");
		});

		InSuite.Add("Tank: damage past the health destroys it and reports the kill", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateTank(InContext, "configs/enemies/Basic.json");
			if ( !CurrentTank )
			{
				return;
			}
			bool bKilled = false;
			CurrentTank->SetOnKilledCallback([&bKilled](GameObject&) { bKilled = true; });
			CurrentTank->ApplyDamage(CurrentTank->GetMaxHealth() + 10, Team::Player);
			InContext.Expect(bKilled, "the kill callback fired");
			InContext.Expect(CurrentTank->IsDestroying(), "the tank is being removed");
			InContext.ExpectEqual(CurrentTank->GetHealth(), 0, "health is 0");
		});

		InSuite.Add("Tank: firing uses ammo and needs a reload", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateTank(InContext, PlayerTankPath());
			if ( !CurrentTank )
			{
				return;
			}
			const int CurrentAmmo = CurrentTank->GetAmmo();
			InContext.Expect(CurrentAmmo > 0, "starts with ammo");
			InContext.Expect(CurrentTank->Shoot(), "the first shot is fired");
			InContext.ExpectEqual(CurrentTank->GetAmmo(), CurrentAmmo - 1, "one shell used");
			InContext.Expect(!CurrentTank->Shoot(), "no second shot while reloading");
			InContext.Expect(CurrentTank->GetReloadProgress() < 1.0f, "reload in progress");

			auto CurrentBullet = GameContext::Get().GetManager<ObjectsFactory>().FindObjectIf([](GameObject& InObject) { return InObject.GetType() == ObjectType::Bullet; });
			InContext.Expect(CurrentBullet != nullptr, "a bullet exists");
			InContext.Expect(CurrentBullet && CurrentBullet->GetTeam() == Team::Player, "the bullet belongs to the shooter's team");
		});

		InSuite.Add("Tank: an enemy has unlimited ammo", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateTank(InContext, "configs/enemies/Basic.json");
			if ( CurrentTank )
			{
				InContext.Expect(CurrentTank->HasUnlimitedAmmo(), "unlimited ammo");
			}
		});

		InSuite.Add("Tank: the shield skill works once, then recharges", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateTank(InContext, PlayerTankPath());
			if ( !CurrentTank )
			{
				return;
			}
			InContext.Expect(CurrentTank->GetAbilityCount() >= 2, "the player has a second skill");
			InContext.Expect(CurrentTank->UseSecondaryAbility(), "the shield is used");
			InContext.Expect(CurrentTank->GetBuffs().IsInvulnerable(), "the shield is up");
			InContext.Expect(!CurrentTank->UseSecondaryAbility(), "it cannot be used again at once");
			const Ability* Shield = CurrentTank->GetAbility(1);
			InContext.Expect(Shield && Shield->GetReadiness() < 1.0f, "it is recharging");
			InContext.Expect(Shield && Shield->GetCooldownRemaining() > 0.0f, "with time left");
		});

		InSuite.Add("Upgrades: bought levels change the tank's stats", [](TestContext& InContext)
		{
			auto& CurrentProfile = GameContext::Get().GetManager<ProfileManager>().Get();
			const auto Saved = CurrentProfile.Upgrades;

			Tank* Plain = CreateTank(InContext, PlayerTankPath());
			const int CurrentAmmo = Plain ? Plain->GetMaxAmmo() : 0;
			const int CurrentDamage = Plain ? Plain->GetShotDamage(Plain->GetActiveWeapon()) : 0;
			const int Armor = Plain ? Plain->GetArmor() : 0;

			CurrentProfile.Upgrades["ammo_capacity"] = 2;
			CurrentProfile.Upgrades["cannon_damage"] = 1;
			CurrentProfile.Upgrades["armor"] = 3;
			Tank* Upgraded = CreateTank(InContext, PlayerTankPath());
			if ( Upgraded )
			{
				GameContext::Get().GetManager<UpgradeManager>().ApplyTo(*Upgraded);
				InContext.ExpectEqual(Upgraded->GetMaxAmmo(), CurrentAmmo + 10, "ammo capacity +5 x 2");
				InContext.ExpectEqual(Upgraded->GetShotDamage(Upgraded->GetActiveWeapon()), CurrentDamage + 1, "cannon damage +1");
				InContext.ExpectEqual(Upgraded->GetArmor(), Armor + 3, "armor +1 x 3");
				InContext.ExpectEqual(Upgraded->GetAmmo(), Upgraded->GetMaxAmmo(), "the tank is refilled");
			}
			CurrentProfile.Upgrades = Saved; // the profile is never saved in tests, but keep it clean
		});

		InSuite.Add("Upgrades: move speed and max health levels reach the tank", [](TestContext& InContext)
		{
			auto& CurrentProfile = GameContext::Get().GetManager<ProfileManager>().Get();
			const auto Saved = CurrentProfile.Upgrades;

			Tank* Plain = CreateTank(InContext, PlayerTankPath());
			const float Speed = Plain ? Plain->GetStats().Get(StatType::Speed) : 0.0f;
			const int MaxHealth = Plain ? Plain->GetMaxHealth() : 0;

			CurrentProfile.Upgrades["move_speed"] = 2;
			CurrentProfile.Upgrades["max_hp"] = 3;
			Tank* Upgraded = CreateTank(InContext, PlayerTankPath());
			if ( Upgraded )
			{
				GameContext::Get().GetManager<UpgradeManager>().ApplyTo(*Upgraded);
				InContext.Expect(std::fabs(Upgraded->GetStats().Get(StatType::Speed) - Speed * 1.21f) < 0.01f, "move speed x1.1 x 2");
				InContext.ExpectEqual(Upgraded->GetMaxHealth(), MaxHealth + 6, "max health +2 x 3");
				InContext.ExpectEqual(Upgraded->GetHealth(), Upgraded->GetMaxHealth(), "the tank is healed to the new maximum");
			}
			CurrentProfile.Upgrades = Saved;
		});

		InSuite.Add("Upgrades: a branch opens step by step, weapon upgrades touch only their weapon", [](TestContext& InContext)
		{
			auto& CurrentProfile = GameContext::Get().GetManager<ProfileManager>().Get();
			auto& Shop = GameContext::Get().GetManager<UpgradeManager>();
			const auto Saved = CurrentProfile.Upgrades;
			const int SavedCoins = CurrentProfile.Coins;

			CurrentProfile.Upgrades.clear();
			CurrentProfile.Coins = 100000;
			InContext.Expect(!Shop.CanBuy("weapon_shotgun"), "the shotgun waits for the cannon reload");
			InContext.Expect(!Shop.CanBuy("weapon_heavy_cannon"), "the heavy cannon waits for the cannon damage");
			CurrentProfile.Upgrades["cannon_reload"] = 1;
			InContext.Expect(Shop.CanBuy("weapon_shotgun"), "the shotgun opens after one cannon reload level");
			InContext.Expect(!Shop.CanBuy("shotgun_damage"), "shotgun damage waits for the shotgun reload");
			CurrentProfile.Upgrades["cannon_damage"] = 1;
			InContext.Expect(!Shop.CanBuy("weapon_heavy_cannon"), "the heavy cannon needs cannon damage level 2");
			CurrentProfile.Upgrades["cannon_damage"] = 2;
			InContext.Expect(Shop.CanBuy("weapon_heavy_cannon"), "the heavy cannon opens at cannon damage level 2");

			Tank* Plain = CreateTank(InContext, PlayerTankPath());
			CurrentProfile.Upgrades.clear();
			CurrentProfile.Upgrades["weapon_shotgun"] = 1;
			CurrentProfile.Upgrades["shotgun_reload"] = 1;
			CurrentProfile.Upgrades["shotgun_damage"] = 2;
			Tank* Upgraded = CreateTank(InContext, PlayerTankPath());
			if ( Plain && Upgraded )
			{
				Shop.ApplyTo(*Upgraded);
				InContext.ExpectEqual(Upgraded->GetShotDamage(Upgraded->GetActiveWeapon()), Plain->GetShotDamage(Plain->GetActiveWeapon()),
					"the cannon keeps its damage");
				InContext.Expect(Upgraded->SelectWeapon(Upgraded->GetWeaponCount() - 1), "the bought shotgun can be selected");
				InContext.ExpectEqual(Upgraded->GetActiveWeapon().Id, std::string("shotgun"), "the last weapon is the shotgun");
				InContext.ExpectEqual(Upgraded->GetActiveWeapon().DamageBonus, 2, "shotgun damage +1 x 2");
				InContext.Expect(std::fabs(Upgraded->GetActiveWeapon().ReloadScale - 0.85f) < 0.001f, "shotgun reload x0.85");
			}
			CurrentProfile.Upgrades = Saved;
			CurrentProfile.Coins = SavedCoins;
		});
	}

	bool HasFrame(const std::string& InFrame)
	{
		return cocos2d::SpriteFrameCache::getInstance()->getSpriteFrameByName(InFrame) != nullptr;
	}

	bool Contains(const std::vector<std::string>& InList, std::string_view InItem)
	{
		return std::find(InList.begin(), InList.end(), InItem) != InList.end();
	}

	// The frames an object config shows: its texture, its damage stages and its animation.
	std::vector<std::string> FramesOf(const std::string& InConfigId)
	{
		const cocos2d::ValueMap& ConfigData = GameContext::Get().GetManager<ConfigReader>().GetObjectData(InConfigId).Values;
		std::vector<std::string> CurrentFrames;
		CurrentFrames.push_back(DataConverter::GetString(ConfigData, "texture_path", ""));

		auto DamageIt = ConfigData.find("damage_frames");
		if ( DamageIt != ConfigData.end() && DamageIt->second.getType() == cocos2d::Value::Type::VECTOR )
		{
			for ( const auto& CurrentFrame : DamageIt->second.asValueVector() )
			{
				CurrentFrames.push_back(CurrentFrame.asString());
			}
		}

		auto AnimationIt = ConfigData.find("move_animation");
		if ( AnimationIt != ConfigData.end() && AnimationIt->second.getType() == cocos2d::Value::Type::MAP )
		{
			const cocos2d::ValueMap& CurrentAnimation = AnimationIt->second.asValueMap();
			const std::string Pattern = DataConverter::GetString(CurrentAnimation, "name", "");
			for ( int Index = 1; Index <= DataConverter::GetInt(CurrentAnimation, "frames_count", 0); ++Index )
			{
				CurrentFrames.push_back(cocos2d::StringUtils::format(Pattern.c_str(), Index));
			}
		}
		return CurrentFrames;
	}

	void ExpectFrames(TestContext& InContext, const std::vector<std::string>& InFrames, const std::string& InWhere)
	{
		for ( const auto& CurrentFrame : InFrames )
		{
			InContext.Expect(HasFrame(CurrentFrame), InWhere + ": frame '" + CurrentFrame + "' is in a loaded atlas");
		}
	}

	void AddResourceTests(TestSuite& InSuite)
	{
		InSuite.Add("Resources: the shared atlases have every frame the configs and the HUD name", [](TestContext& InContext)
		{
			const ResourceManifest Manifest;
			InContext.Expect(!Manifest.GetGlobalAtlases().empty(), "there are shared atlases");
			for ( const auto& CurrentAtlas : Manifest.GetGlobalAtlases() )
			{
				InContext.Expect(GameContext::Get().GetManager<ResourceManager>().IsAtlasLoaded(CurrentAtlas), CurrentAtlas + " is loaded at start");
			}

			ExpectFrames(InContext, FramesOf(PlayerTankPath()), PlayerTankPath());
			ExpectFrames(InContext, FramesOf(PathOf(ids::configs::Bullet, model::ResourceKind::Config)), "bullet config");
			ExpectFrames(InContext, FramesOf("configs/bullet_mg.json"), "configs/bullet_mg.json");
			const auto CurrentWeapons = TankLoader::ReadWeapons(GameContext::Get().GetManager<ConfigReader>().GetConfig(PlayerTankPath()), PathOf(ids::configs::Bullet, model::ResourceKind::Config));
			for ( const auto& Weapon : CurrentWeapons )
			{
				ExpectFrames(InContext, { Weapon.Icon }, "weapon " + Weapon.Id);
				InContext.Expect(GameContext::Get().GetManager<ConfigReader>().HasConfig(Weapon.BulletConfig), "weapon " + Weapon.Id + ": its bullet config exists");
			}
			for ( const auto& CurrentBonus : { "configs/bonus_ammo.json", "configs/bonus_repair.json", "configs/bonus_rapid_fire.json", "configs/bonus_shield.json" } )
			{
				ExpectFrames(InContext, FramesOf(CurrentBonus), CurrentBonus);
			}
			ExpectFrames(InContext, { "boom1", "boom2", "boom3", "boom4", "boom5", "spawn_marker", "icon_shoot" }, "effects");

			const cocos2d::ValueMap& CurrentBuffs = GameContext::Get().GetManager<ConfigReader>().GetObjectData("configs/buffs.json").Values;
			for ( const auto& Buff : CurrentBuffs.at("buffs").asValueVector() )
			{
				const std::string CurrentId = DataConverter::GetString(Buff.asValueMap(), "id", "");
				ExpectFrames(InContext, { GameContext::Get().GetManager<BuffCatalog>().Find(CurrentId)->Icon }, "buff " + CurrentId);
			}
		});

		InSuite.Add("Resources: every enemy and tileset atlas has the frames its configs name", [](TestContext& InContext)
		{
			const ResourceManifest Manifest;
			for ( const auto& Enemy : GameContext::Get().GetManager<EnemyCatalog>().GetAll() )
			{
				InContext.Expect(!Enemy.Atlas.empty(), Enemy.Id + ": names its atlas");
				ResourceHolder CurrentAtlas({ Enemy.Atlas });
				ExpectFrames(InContext, FramesOf(Enemy.ConfigId), Enemy.Id);
			}

			const std::vector<std::string> CurrentTilesets = Manifest.GetTilesetAtlases();
			InContext.Expect(CurrentTilesets.size() >= 2, "there are several tilesets");
			for ( const auto& CurrentName : { "field", "desert" } )
			{
				const std::string CurrentTileset = Manifest.GetTilesetAtlas(CurrentName);
				ResourceHolder CurrentAtlas({ CurrentTileset });
				InContext.Expect(GameContext::Get().GetManager<ResourceManager>().IsAtlasLoaded(CurrentTileset), CurrentTileset + " loads");

				std::vector<std::string> CurrentFrames = { "ground_1", "ground_2", "ground_3", "ground_4" };
				for ( const auto& CurrentWall : { "configs/wall_brick.json", "configs/wall_stone.json", "configs/wall_wood.json" } )
				{
					const auto WallFrames = FramesOf(CurrentWall);
					CurrentFrames.insert(CurrentFrames.end(), WallFrames.begin(), WallFrames.end());
				}
				for ( auto& CurrentFrame : CurrentFrames )
				{
					CurrentFrame = ResourceManifest::GetTilesetFrame(CurrentName, CurrentFrame);
				}
				ExpectFrames(InContext, CurrentFrames, CurrentTileset);
			}
		});

		InSuite.Add("Resources: two tilesets loaded together keep their own frames", [](TestContext& InContext)
		{
			const ResourceManifest Manifest;
			{
				ResourceHolder Field({ Manifest.GetTilesetAtlas("field") });
				{
					ResourceHolder Desert({ Manifest.GetTilesetAtlas("desert") });
				}
				InContext.Expect(HasFrame(ResourceManifest::GetTilesetFrame("field", "wall_brick")), "unloading desert keeps the field frames");
			}
			InContext.Expect(!HasFrame(ResourceManifest::GetTilesetFrame("field", "wall_brick")), "field is unloaded at the end");
		});

		InSuite.Add("Resources: a level loads only its tileset and the enemies its waves can send", [](TestContext& InContext)
		{
			const ResourceManifest Manifest;
			LevelData First;
			LevelData Last;
			InContext.Expect(LevelLoader::Load(LevelLoader::BuiltinId(0), First), "level 1 loads");
			InContext.Expect(LevelLoader::Load(LevelLoader::BuiltinId(5), Last), "level 6 loads");

			const auto FirstAtlases = Manifest.GetLevelAtlases(First);
			InContext.Expect(Contains(FirstAtlases, Manifest.GetTilesetAtlas(First.Tileset)), "level 1: its tileset");
			InContext.Expect(Contains(FirstAtlases, GameContext::Get().GetManager<EnemyCatalog>().Find("Basic")->Atlas), "level 1: its enemy");
			InContext.ExpectEqual(FirstAtlases.size(), size_t(2), "level 1: the tileset and one enemy, nothing else");

			const auto LastAtlases = Manifest.GetLevelAtlases(Last);
			InContext.Expect(Contains(LastAtlases, GameContext::Get().GetManager<EnemyCatalog>().Find("Boss")->Atlas), "level 6: the boss");
			InContext.Expect(!Contains(LastAtlases, GameContext::Get().GetManager<EnemyCatalog>().Find("Basic")->Atlas), "level 6: no excluded enemy");
			InContext.Expect(Manifest.GetTilesetAtlas(First.Tileset) != Manifest.GetTilesetAtlas(Last.Tileset), "levels 1 and 6 look different");
			InContext.Expect(!Manifest.GetTilesetAtlas("").empty(), "a level without a tileset gets the default one");
		});

		InSuite.Add("Resources: the UI fonts ship with their licences", [](TestContext& InContext)
		{
			auto Files = cocos2d::FileUtils::getInstance();
			for ( const auto& File : { PathOf(ids::fonts::Decorative, model::ResourceKind::Font), PathOf(ids::fonts::Plain, model::ResourceKind::Font), std::string("fonts/OFL-PressStart2P.txt"), std::string("fonts/OFL-PTSans.txt") } )
			{
				InContext.Expect(Files->isFileExist(File), std::string(File) + " exists");
			}
		});

		InSuite.Add("Data: every language has every string of the English one", [](TestContext& InContext)
		{
			const cocos2d::ValueMap& English = GameContext::Get().GetManager<ConfigReader>().GetObjectData("configs/strings/english.json").Values;
			InContext.Expect(!English.empty(), "English strings are loaded");
			for ( const auto& StoredLanguage : { "configs/strings/ukrainian.json", "configs/strings/russian.json" } )
			{
				const cocos2d::ValueMap& CurrentStrings = GameContext::Get().GetManager<ConfigReader>().GetObjectData(StoredLanguage).Values;
				for ( const auto& CurrentEntry : English )
				{
					InContext.Expect(CurrentStrings.count(CurrentEntry.first) > 0, std::string(StoredLanguage) + ": has " + CurrentEntry.first);
				}
			}
		});

		InSuite.Add("Resources: an atlas stays loaded while any scope needs it", [](TestContext& InContext)
		{
			const std::string CurrentAtlas = ResourceManifest().GetTilesetAtlas("desert");
			const std::string CurrentFrame = ResourceManifest::GetTilesetFrame("desert", "ground_1");
			InContext.Expect(!GameContext::Get().GetManager<ResourceManager>().IsAtlasLoaded(CurrentAtlas), "not loaded before");
			{
				ResourceHolder Outer({ CurrentAtlas });
				{
					ResourceHolder Inner({ CurrentAtlas });
					InContext.Expect(HasFrame(CurrentFrame), "loaded by two scopes");
				}
				InContext.Expect(GameContext::Get().GetManager<ResourceManager>().IsAtlasLoaded(CurrentAtlas) && HasFrame(CurrentFrame), "still loaded by the other scope");
			}
			InContext.Expect(!GameContext::Get().GetManager<ResourceManager>().IsAtlasLoaded(CurrentAtlas), "unloaded after the last scope");
			InContext.Expect(!HasFrame(CurrentFrame), "its frames are gone");
		});
	}
}

namespace
{
	Wall* CreateWall(TestContext& InContext, const std::string& InConfig, const cocos2d::Vec2& InPosition)
	{
		const cocos2d::ValueMap& ConfigData = GameContext::Get().GetManager<ConfigReader>().GetObjectData(InConfig).Values;
		cocos2d::ValueVector Position;
		Position.push_back(cocos2d::Value(InPosition.x));
		Position.push_back(cocos2d::Value(InPosition.y));
		cocos2d::ValueMap Overrides;
		Overrides["position"] = Position;
		Overrides["texture_path"] = ResourceManifest::GetTilesetFrame("field", DataConverter::GetString(ConfigData, "texture_path", ""));
		auto CurrentWall = GameContext::Get().GetManager<ObjectsFactory>().CreateFromConfig<Wall>(InConfig, InContext.GetSandbox(), Overrides);
		InContext.Expect(CurrentWall != nullptr, "wall " + InConfig + " is created");
		return CurrentWall;
	}

	// Rect of a box in a wall's cells: column/row 0 is the wall's bottom-left cell.
	cocos2d::Rect CellsRect(const Wall& InWall, const float InLeft, const float InBottom, const float InRight, const float InTop)
	{
		const cocos2d::Rect Box = InWall.GetCollisionRect();
		const float Cell = Box.size.width / InWall.GetMask().GetCellsPerSide();
		return cocos2d::Rect(Box.getMinX() + InLeft * Cell, Box.getMinY() + InBottom * Cell, (InRight - InLeft) * Cell, (InTop - InBottom) * Cell);
	}

	void AddWallTests(TestSuite& InSuite)
	{
		InSuite.Add("Walls: a hit from below chips the bottom rows, the next one goes deeper", [](TestContext& InContext)
		{
			model::WallMask CurrentMask(4, 1);
			InContext.ExpectEqual(CurrentMask.Chip(model::HitSide::Bottom, 0.0f, 4.0f, 2, 1), 8, "two rows of four cells");
			InContext.Expect(!CurrentMask.IsSolid(0, 0) && !CurrentMask.IsSolid(3, 1), "the bottom rows are gone");
			InContext.Expect(CurrentMask.IsSolid(0, 2) && CurrentMask.IsSolid(3, 3), "the top rows stand");
			InContext.ExpectEqual(CurrentMask.Chip(model::HitSide::Bottom, 0.0f, 4.0f, 2, 1), 8, "the next hit takes the rest");
			InContext.Expect(CurrentMask.IsEmpty(), "nothing is left");
		});

		InSuite.Add("Walls: a narrow hit chips only its line; a tough cell needs several hits", [](TestContext& InContext)
		{
			model::WallMask CurrentMask(4, 2);
			InContext.ExpectEqual(CurrentMask.Chip(model::HitSide::Left, 1.0f, 2.0f, 1, 1), 0, "a damaged cell still stands");
			InContext.Expect(CurrentMask.IsSolid(0, 1) && CurrentMask.GetCellHealth(0, 1) == 1, "the cell lost half of its health");
			InContext.ExpectEqual(CurrentMask.Chip(model::HitSide::Left, 1.0f, 2.0f, 1, 1), 1, "the second hit removes it");
			InContext.Expect(!CurrentMask.IsSolid(0, 1) && CurrentMask.IsSolid(1, 1) && CurrentMask.IsSolid(0, 0) && CurrentMask.IsSolid(0, 2), "only that cell is gone");
			CurrentMask.Chip(model::HitSide::Right, 0.0f, 0.99f, 1, 2);
			InContext.Expect(!CurrentMask.IsSolid(3, 0) && CurrentMask.IsSolid(2, 0), "a hit from the right chips the right edge");
		});

		InSuite.Add("Walls: only standing cells block, and a swept bullet stops at the first of them", [](TestContext& InContext)
		{
			model::WallMask CurrentMask(4, 1);
			CurrentMask.Chip(model::HitSide::Bottom, 0.0f, 4.0f, 2, 1);
			InContext.Expect(!CurrentMask.Overlaps(0.0f, 0.0f, 4.0f, 1.9f), "the cleared bottom half is free");
			InContext.Expect(CurrentMask.Overlaps(0.0f, 1.5f, 4.0f, 2.5f), "the top half blocks");

			float T = 0.0f;
			InContext.Expect(CurrentMask.Sweep(2.0f, -1.0f, 2.0f, 5.0f, 0.1f, T), "a bullet from below hits");
			InContext.ExpectNear(T, (2.0f - 0.1f + 1.0f) / 6.0f, 0.001f, "at the first standing row");

			model::WallMask Column(4, 1);
			Column.Chip(model::HitSide::Bottom, 1.0f, 2.0f, 4, 1);
			InContext.Expect(!Column.Sweep(1.5f, -1.0f, 1.5f, 5.0f, 0.2f, T), "a bullet flies through a cleared column");
			InContext.Expect(Column.Sweep(2.5f, -1.0f, 2.5f, 5.0f, 0.2f, T), "but not through its neighbour");
		});

		InSuite.Add("Walls: a tank fits where a shell cleared the wall; two shells destroy a brick tile", [](TestContext& InContext)
		{
			const ResourceHolder CurrentTileset({ ResourceManifest().GetTilesetAtlas("field") });
			Wall* CurrentWall = CreateWall(InContext, "configs/wall_brick.json", cocos2d::Vec2(TileSize / 2, TileSize / 2));
			if ( !CurrentWall )
			{
				return;
			}
			const cocos2d::Rect Box = CurrentWall->GetCollisionRect();
			InContext.ExpectNear(Box.size.width, TileSize, 0.5f, "a brick wall is one tile wide");

			const cocos2d::Rect Shell(Box.getMinX(), Box.getMinY() - 2.0f, TileSize, 3.0f);
			CurrentWall->Chip(Shell, model::HitSide::Bottom, 2, 2, false);
			InContext.Expect(!CurrentWall->Blocks(CellsRect(*CurrentWall, 0.0f, 0.0f, 4.0f, 1.95f)), "the lower half is open");
			InContext.Expect(CurrentWall->Blocks(CellsRect(*CurrentWall, 0.5f, 1.5f, 3.5f, 2.5f)), "the upper half still blocks");
			InContext.Expect(!CurrentWall->IsDestroying(), "the wall still stands");

			const cocos2d::Rect Deeper(Box.getMinX(), Box.getMinY() + TileSize / 2 - 2.0f, TileSize, 3.0f);
			CurrentWall->Chip(Deeper, model::HitSide::Bottom, 2, 2, false);
			InContext.Expect(CurrentWall->GetMask().IsEmpty() && CurrentWall->IsDestroying(), "the second shell destroys it");
		});

		InSuite.Add("Walls: a shot between two walls chips both; stone needs a stone-breaking shot", [](TestContext& InContext)
		{
			const ResourceHolder CurrentTileset({ ResourceManifest().GetTilesetAtlas("field") });
			Wall* Left = CreateWall(InContext, "configs/wall_brick.json", cocos2d::Vec2(TileSize / 2, TileSize / 2));
			Wall* Right = CreateWall(InContext, "configs/wall_brick.json", cocos2d::Vec2(TileSize * 1.5f, TileSize / 2));
			Wall* CurrentStone = CreateWall(InContext, "configs/wall_stone.json", cocos2d::Vec2(TileSize * 3.5f, TileSize / 2));
			if ( !Left || !Right || !CurrentStone )
			{
				return;
			}
			const float Cell = TileSize / 4;
			const cocos2d::Rect Between(TileSize - Cell, -2.0f, TileSize, 3.0f);
			const int LeftCells = Left->Chip(Between, model::HitSide::Bottom, 2, 2, false);
			const int RightCells = Right->Chip(Between, model::HitSide::Bottom, 2, 2, false);
			InContext.ExpectEqual(LeftCells, 2, "one column of the left wall, two cells deep");
			InContext.ExpectEqual(RightCells, 6, "three columns of the right wall");

			const cocos2d::Rect OnStone(TileSize * 3, -2.0f, TileSize, 3.0f);
			InContext.ExpectEqual(CurrentStone->Chip(OnStone, model::HitSide::Bottom, 2, 2, false), 0, "a normal shell does not chip stone");
			InContext.Expect(CurrentStone->Chip(OnStone, model::HitSide::Bottom, 2, 4, true) > 0, "a stone-breaking one does");
		});
	}

	void AddWeaponTests(TestSuite& InSuite)
	{
		InSuite.Add("Weapons: the player switches between the cannon and the machine gun", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateTank(InContext, PlayerTankPath());
			if ( !CurrentTank )
			{
				return;
			}
			InContext.ExpectEqual(CurrentTank->GetWeaponCount(), size_t(2), "two weapons");
			InContext.ExpectEqual(CurrentTank->GetActiveWeapon().Id, std::string("cannon"), "the cannon first");
			CurrentTank->OnCommandReceived(GameCommand(CommandType::SwitchWeapon));
			InContext.ExpectEqual(CurrentTank->GetActiveWeapon().Id, std::string("machine_gun"), "the switch command takes the machine gun");
			InContext.Expect(CurrentTank->SwitchWeapon(), "switching again works");
			InContext.ExpectEqual(CurrentTank->GetActiveWeapon().Id, std::string("cannon"), "and comes back to the cannon");
		});

		InSuite.Add("Weapons: the machine gun fires without shells, weaker and faster", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateTank(InContext, PlayerTankPath());
			if ( !CurrentTank )
			{
				return;
			}
			CurrentTank->SetAmmo(0);
			InContext.Expect(!CurrentTank->CanFireActiveWeapon() && !CurrentTank->Shoot(), "the cannon is empty");
			const float CannonReload = CurrentTank->GetStats().Get(StatType::ReloadTime);

			CurrentTank->SwitchWeapon();
			InContext.Expect(CurrentTank->CanFireActiveWeapon(), "the machine gun needs no shells");
			InContext.Expect(CurrentTank->Shoot(), "it fires");
			InContext.ExpectEqual(CurrentTank->GetAmmo(), 0, "no shells were used");
			InContext.Expect(CurrentTank->GetReloadRemaining() > 0.0f && CurrentTank->GetReloadRemaining() < CannonReload, "it reloads faster than the cannon");

			auto CurrentBullet = GameContext::Get().GetManager<ObjectsFactory>().FindObjectIf([](GameObject& InObject) { return InObject.GetType() == ObjectType::Bullet; });
			InContext.Expect(CurrentBullet != nullptr, "a bullet exists");
			if ( CurrentBullet )
			{
				InContext.Expect(CurrentBullet->GetDamage() < CurrentTank->GetDamage(), "it hits weaker than a shell");
			}
		});

		InSuite.Add("Weapons: an enemy without a weapons list has one cannon", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateTank(InContext, "configs/enemies/Basic.json");
			if ( CurrentTank )
			{
				InContext.ExpectEqual(CurrentTank->GetWeaponCount(), size_t(1), "one weapon");
				InContext.Expect(!CurrentTank->SwitchWeapon(), "nothing to switch to");
				InContext.Expect(CurrentTank->GetActiveWeapon().bUsesAmmo, "a cannon");
			}
		});
	}
}

namespace
{
	constexpr float FrameTime = 1.0f / 60.0f;

	void Advance(const float InSeconds)
	{
		auto Scheduler = cocos2d::Director::getInstance()->getScheduler();
		for ( float Time = 0.0f; Time < InSeconds; Time += FrameTime )
		{
			Scheduler->update(FrameTime);
		}
	}

	Tank* CreateTankAt(TestContext& InContext, const std::string& InConfig, const cocos2d::Vec2& InPosition)
	{
		Tank* CurrentTank = CreateTank(InContext, InConfig);
		if ( CurrentTank )
		{
			CurrentTank->GetVisualNode()->setPosition(InPosition);
			CurrentTank->SetBounds(cocos2d::Rect(0, 0, GridColumns * TileSize, GridRows * TileSize));
		}
		return CurrentTank;
	}

	float OverlapArea(const Tank& InFirst, const Tank& InSecond)
	{
		const cocos2d::Rect A = InFirst.GetCollisionRect();
		const cocos2d::Rect B = InSecond.GetCollisionRect();
		const float CurrentWidth = std::min(A.getMaxX(), B.getMaxX()) - std::max(A.getMinX(), B.getMinX());
		const float CurrentHeight = std::min(A.getMaxY(), B.getMaxY()) - std::max(A.getMinY(), B.getMinY());
		return CurrentWidth > 0.0f && CurrentHeight > 0.0f ? CurrentWidth * CurrentHeight : 0.0f;
	}

	void AddUpgradeTreeTests(TestSuite& InSuite)
	{
		using model::Vec2f;

		InSuite.Add("Upgrade tree: nodes sit on their branch steps and link to the parent or the tank part", [](TestContext& InContext)
		{
			model::UpgradeTree Tree;
			Tree.SetBranches({ { "gun", Vec2f(0.0f, 0.0f), Vec2f(10.0f, 100.0f), Vec2f(0.0f, 50.0f) },
				{ "hull", Vec2f(5.0f, 5.0f), Vec2f(100.0f, 0.0f), Vec2f(40.0f, 0.0f) } });
			const std::vector<std::string> Skipped = Tree.Build({ { "unlock", "gun", 0, "" }, { "reload", "gun", 1, "unlock" },
				{ "armor", "hull", 0, "reload" }, { "lost", "nowhere", 0, "" } });
			InContext.ExpectEqual(Tree.GetNodeCount(), 3, "the node on an unknown branch is left out");
			InContext.ExpectEqual(static_cast<int>(Skipped.size()), 1, "and reported");
			InContext.Expect(Tree.GetPosition(Tree.FindNode("reload")).IsNear(Vec2f(10.0f, 150.0f), 0.01f), "step 1 is one step from the anchor");
			InContext.Expect(Tree.GetLinkStart(Tree.FindNode("reload")).IsNear(Vec2f(10.0f, 100.0f), 0.01f), "a link starts at the parent on the same branch");
			InContext.Expect(Tree.GetLinkStart(Tree.FindNode("unlock")).IsNear(Vec2f(0.0f, 0.0f), 0.01f), "a first node links to its tank part");
			InContext.ExpectEqual(Tree.GetParent(Tree.FindNode("armor")), -1, "a parent on another branch draws no link");
			InContext.Expect(Tree.GetLinkStart(Tree.FindNode("armor")).IsNear(Vec2f(5.0f, 5.0f), 0.01f), "so it links to its own tank part");
		});

		InSuite.Add("Upgrade tree: arrows move the focus to the nearest node that way", [](TestContext& InContext)
		{
			using model::NodeNavigator;
			const std::vector<Vec2f> Points{ { 0.0f, 0.0f }, { 0.0f, 100.0f }, { 100.0f, 10.0f }, { 60.0f, 200.0f }, { -100.0f, -5.0f } };
			InContext.ExpectEqual(NodeNavigator::FindNext(Points, {}, 0, Vec2f(0.0f, 1.0f)), 1, "up: straight above beats far and to the side");
			InContext.ExpectEqual(NodeNavigator::FindNext(Points, {}, 0, Vec2f(1.0f, 0.0f)), 2, "right");
			InContext.ExpectEqual(NodeNavigator::FindNext(Points, {}, 0, Vec2f(-1.0f, 0.0f)), 4, "left");
			InContext.ExpectEqual(NodeNavigator::FindNext(Points, {}, 4, Vec2f(0.0f, -1.0f)), 4, "nothing below: the focus stays");
			InContext.ExpectEqual(NodeNavigator::FindNext(Points, { true, false, true, true, true }, 0, Vec2f(0.0f, 1.0f)), 3, "hidden nodes are skipped");
		});
	}

	void AddTurnTests(TestSuite& InSuite)
	{
		using model::TurnState;

		InSuite.Add("Turning: angles take the shorter way and wrap around", [](TestContext& InContext)
		{
			InContext.ExpectNear(TurnState::Difference(0.0f, 270.0f), -90.0f, 0.001f, "up to left is a left turn");
			InContext.ExpectNear(TurnState::Difference(270.0f, 0.0f), 90.0f, 0.001f, "left to up is a right turn");
			InContext.ExpectNear(TurnState::Normalize(-90.0f), 270.0f, 0.001f, "negative angles wrap");
			InContext.ExpectNear(TurnState::Normalize(450.0f), 90.0f, 0.001f, "large angles wrap");
		});

		InSuite.Add("Turning: a turn eases out and ends on its target in its time", [](TestContext& InContext)
		{
			TurnState Turn;
			Turn.Start(0.0f, 90.0f, 0.2f);
			InContext.Expect(Turn.IsTurning(), "turning");
			InContext.Expect(!Turn.Update(0.1f), "not finished halfway");
			InContext.Expect(Turn.GetAngle() > 45.0f && Turn.GetAngle() < 90.0f, "ease-out: more than half the way at half the time");
			InContext.Expect(Turn.Update(0.1f), "finishes on time");
			InContext.ExpectNear(Turn.GetAngle(), 90.0f, 0.001f, "on target");
			InContext.Expect(!Turn.IsTurning(), "stopped");
			Turn.Start(90.0f, 90.0f, 0.2f);
			InContext.Expect(!Turn.IsTurning(), "the same facing starts no turn");
		});

		InSuite.Add("Turning: a counter-turn goes back to the previous facing, faster when started early", [](TestContext& InContext)
		{
			TurnState Turn;
			Turn.SetEaseRate(1.0f);
			Turn.Start(0.0f, 90.0f, 0.4f);
			Turn.Update(0.1f);
			InContext.Expect(Turn.Reverse(0.2f), "reversed");
			InContext.ExpectNear(Turn.GetTarget(), 0.0f, 0.001f, "heads back to the old facing");
			InContext.ExpectNear(Turn.GetPrevious(), 90.0f, 0.001f, "the abandoned target becomes the previous facing");
			InContext.Expect(!Turn.Update(0.04f), "a quarter undone takes a quarter of the counter-turn time");
			InContext.Expect(Turn.Update(0.02f), "and ends then");
			InContext.ExpectNear(Turn.GetAngle(), 0.0f, 0.001f, "back on the old facing");
			InContext.Expect(!Turn.Reverse(0.2f), "nothing to reverse after the turn");
		});

		InSuite.Add("Turning: the tank cannot fire mid-turn; the shot is queued and fires after the turn", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateTankAt(InContext, PlayerTankPath(), cocos2d::Vec2(TileSize * 4.5f, TileSize * 4.5f));
			if ( !CurrentTank )
			{
				return;
			}
			const int StartAmmo = CurrentTank->GetAmmo();
			CurrentTank->OnCommandReceived(GameCommand(CommandType::MoveRight));
			InContext.Expect(CurrentTank->IsTurning(), "turning right");
			InContext.Expect(!CurrentTank->Shoot() && CurrentTank->IsShotQueued(), "no shot mid-turn, but it is queued");
			InContext.ExpectEqual(CurrentTank->GetAmmo(), StartAmmo, "no shell used yet");
			Advance(1.0f);
			InContext.Expect(!CurrentTank->IsTurning() && !CurrentTank->IsShotQueued(), "the turn ended and the queue is empty");
			InContext.ExpectEqual(CurrentTank->GetAmmo(), StartAmmo - 1, "the queued shot was fired");
			InContext.ExpectNear(CurrentTank->GetDirection().x, 1.0f, 0.001f, "facing right");
		});

		InSuite.Add("Turning: the opposite input mid-turn snaps back to the previous facing", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateTankAt(InContext, PlayerTankPath(), cocos2d::Vec2(TileSize * 4.5f, TileSize * 4.5f));
			if ( !CurrentTank )
			{
				return;
			}
			CurrentTank->OnCommandReceived(GameCommand(CommandType::Stop));
			CurrentTank->OnCommandReceived(GameCommand(CommandType::MoveRight));
			Advance(1.0f / 60.0f);
			CurrentTank->OnCommandReceived(GameCommand(CommandType::MoveForward));
			InContext.Expect(CurrentTank->GetDirection().y > 0.99f, "the direction points back up at once");
			Advance(1.0f);
			InContext.Expect(!CurrentTank->IsTurning(), "the counter-turn ended");
			InContext.ExpectNear(model::TurnState::Normalize(CurrentTank->GetVisualNode()->getRotation()), 0.0f, 0.5f, "facing up again");
		});

		InSuite.Add("Turning: the TurnSpeed stat comes from upgrades and buffs", [](TestContext& InContext)
		{
			StatType Stat = StatType::Damage;
			InContext.Expect(EnumFromName("TurnSpeed", Stat) && Stat == StatType::TurnSpeed, "the stat has a config name");
			Tank* CurrentTank = CreateTank(InContext, PlayerTankPath());
			InContext.Expect(CurrentTank && std::fabs(CurrentTank->GetTurnSpeed() - 1.0f) < 0.001f, "base turn speed is 1");
		});
	}

	int CountBullets()
	{
		int Count = 0;
		GameContext::Get().GetManager<ObjectsFactory>().ForEachObject(ObjectType::Bullet, [&Count](GameObject& InObject)
		{
			Count += InObject.IsDestroying() ? 0 : 1;
		});
		return Count;
	}

	Tank* CreateArmedPlayer(TestContext& InContext, const std::string_view InWeaponId)
	{
		Tank* CurrentTank = CreateTankAt(InContext, PlayerTankPath(), cocos2d::Vec2(TileSize * 4.5f, TileSize * 4.5f));
		if ( !CurrentTank )
		{
			return nullptr;
		}
		InContext.Expect(CurrentTank->UnlockWeapon(InWeaponId), std::string(InWeaponId) + " unlocks");
		CurrentTank->SelectWeapon(CurrentTank->GetWeaponCount() - 1);
		InContext.Expect(CurrentTank->GetActiveWeapon().Id == InWeaponId, std::string(InWeaponId) + " is active");
		return CurrentTank;
	}

	void AddNewWeaponTests(TestSuite& InSuite)
	{
		InSuite.Add("Weapons: new guns are locked until unlocked", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateTank(InContext, PlayerTankPath());
			if ( !CurrentTank )
			{
				return;
			}
			const size_t Usable = CurrentTank->GetWeaponCount();
			InContext.ExpectEqual(Usable, static_cast<size_t>(2), "only the cannon and the machine gun at the start");
			InContext.Expect(CurrentTank->UnlockWeapon("shotgun") && CurrentTank->GetWeaponCount() == Usable + 1, "unlocking adds the gun");
			InContext.Expect(!CurrentTank->UnlockWeapon("shotgun"), "a gun unlocks once");
		});

		InSuite.Add("Weapons: the shotgun fires a fan of pellets that fly a short way", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateArmedPlayer(InContext, "shotgun");
			if ( !CurrentTank )
			{
				return;
			}
			const int Before = CountBullets();
			InContext.Expect(CurrentTank->Fire(), "fires");
			InContext.ExpectEqual(CountBullets(), Before + 3, "three pellets");
			Advance(1.0f);
			InContext.ExpectEqual(CountBullets(), Before, "the pellets are gone after their short range");
		});

		InSuite.Add("Weapons: flak fires a burst of three for one shell", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateArmedPlayer(InContext, "flak");
			if ( !CurrentTank )
			{
				return;
			}
			const int Ammo = CurrentTank->GetAmmo();
			const int Before = CountBullets();
			InContext.Expect(CurrentTank->Fire(), "fires");
			Advance(0.2f);
			InContext.ExpectEqual(CountBullets(), Before + 3, "three shots in a row");
			InContext.ExpectEqual(CurrentTank->GetAmmo(), Ammo - 1, "one shell for the burst");
		});

		InSuite.Add("Weapons: a mine is dropped behind the tank and lies still", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateArmedPlayer(InContext, "mine");
			if ( !CurrentTank || !CurrentTank->Fire() )
			{
				return;
			}
			GameObject* Mine = GameContext::Get().GetManager<ObjectsFactory>().FindObjectIf([](GameObject& InObject) { return InObject.GetType() == ObjectType::Bullet; });
			const float TankY = CurrentTank->GetVisualNode()->getPositionY();
			InContext.Expect(Mine && Mine->GetVisualNode()->getPositionY() < TankY, "behind a tank facing up");
			const float MineY = Mine ? Mine->GetVisualNode()->getPositionY() : 0.0f;
			Advance(0.5f);
			InContext.Expect(Mine && !Mine->IsDestroying() && std::fabs(Mine->GetVisualNode()->getPositionY() - MineY) < 0.01f, "still there, not moving");
		});

		InSuite.Add("Weapons: a ricochet shell bounces off a wall once, then chips it", [](TestContext& InContext)
		{
			const ResourceHolder CurrentTileset({ ResourceManifest().GetTilesetAtlas("field") });
			Tank* CurrentTank = CreateArmedPlayer(InContext, "ricochet");
			if ( !CurrentTank )
			{
				return;
			}
			const cocos2d::Vec2 TankAt = CurrentTank->GetVisualNode()->getPosition();
			Wall* CurrentWall = CreateWall(InContext, "configs/wall_stone.json", TankAt + cocos2d::Vec2(0.0f, TileSize * 2.0f));
			if ( !CurrentWall || !CurrentTank->Fire() )
			{
				return;
			}
			GameObject* Shell = GameContext::Get().GetManager<ObjectsFactory>().FindObjectIf([](GameObject& InObject) { return InObject.GetType() == ObjectType::Bullet; });
			Advance(0.15f);
			InContext.Expect(Shell && !Shell->IsDestroying() && std::fabs(model::TurnState::Normalize(Shell->GetVisualNode()->getRotation()) - 180.0f) < 0.5f, "flies back after the first wall hit");
		});
	}

	void AddMovementTests(TestSuite& InSuite)
	{
		InSuite.Add("Weapons: a held trigger fires the cannon once, until auto-fire is bought for it", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateTankAt(InContext, PlayerTankPath(), cocos2d::Vec2(TileSize * 4.5f, TileSize * 4.5f));
			if ( !CurrentTank || !CurrentTank->SelectWeapon(0) )
			{
				return;
			}
			const int StartAmmo = CurrentTank->GetAmmo();
			InContext.Expect(!CurrentTank->CanAutoFire(), "the cannon does not auto-fire by default");
			CurrentTank->OnCommandReceived(GameCommand(CommandType::StartFiring));
			Advance(CurrentTank->GetReloadRemaining() * 3.0f);
			InContext.ExpectEqual(CurrentTank->GetAmmo(), StartAmmo - 1, "one shell for one press");
			CurrentTank->OnCommandReceived(GameCommand(CommandType::StopFiring));

			InContext.Expect(CurrentTank->EnableAutoFire("cannon") && CurrentTank->CanAutoFire(), "the upgrade turns auto-fire on");
			InContext.Expect(!CurrentTank->EnableAutoFire("no_such_gun"), "an unknown weapon is reported");
		});

		InSuite.Add("Weapons: the auto-fire upgrade reaches the tank from the shop", [](TestContext& InContext)
		{
			const auto& Upgrades = GameContext::Get().GetManager<UpgradeManager>().GetUpgrades();
			const bool bHasCannonUpgrade = std::any_of(Upgrades.begin(), Upgrades.end(), [](const UpgradeDef& InUpgrade)
			{
				return InUpgrade.Id == "auto_fire_cannon" && InUpgrade.AutoFireWeapon == "cannon" && InUpgrade.MaxLevel == 1;
			});
			InContext.Expect(bHasCannonUpgrade, "auto_fire_cannon is a one-level upgrade for the cannon");
		});

		InSuite.Add("Weapons: a held trigger keeps an auto-fire gun firing each time it is ready, until released", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateTankAt(InContext, PlayerTankPath(), cocos2d::Vec2(TileSize * 4.5f, TileSize * 4.5f));
			if ( !CurrentTank )
			{
				return;
			}
			CurrentTank->EnableAutoFire("cannon");
			const int StartAmmo = CurrentTank->GetAmmo();
			CurrentTank->OnCommandReceived(GameCommand(CommandType::StartFiring));
			InContext.Expect(CurrentTank->IsTriggerHeld() && CurrentTank->GetAmmo() == StartAmmo - 1, "the first shell leaves at once");

			const float Reload = CurrentTank->GetReloadRemaining();
			Advance(Reload * 2.5f);
			InContext.Expect(CurrentTank->GetAmmo() <= StartAmmo - 3, "more shells follow while the trigger is held");

			CurrentTank->OnCommandReceived(GameCommand(CommandType::StopFiring));
			const int AfterRelease = CurrentTank->GetAmmo();
			Advance(Reload * 2.5f);
			InContext.Expect(!CurrentTank->IsTriggerHeld() && CurrentTank->GetAmmo() == AfterRelease, "no more shots after the release");
		});

		InSuite.Add("Movement: two tanks stuck inside each other can drive apart, but not deeper in", [](TestContext& InContext)
		{
			const cocos2d::Vec2 Center(TileSize * 4.5f, TileSize * 4.5f);
			Tank* First = CreateTankAt(InContext, PlayerTankPath(), Center);
			Tank* Second = CreateTankAt(InContext, "configs/enemies/Basic.json", Center + cocos2d::Vec2(10.0f, 0.0f));
			if ( !First || !Second )
			{
				return;
			}
			InContext.Expect(OverlapArea(*First, *Second) > 0.0f, "they overlap at the start");
			InContext.Expect(First->CanMoveTo(Center - cocos2d::Vec2(8.0f, 0.0f)), "a step away from the other tank is allowed");
			InContext.Expect(!First->CanMoveTo(Center + cocos2d::Vec2(8.0f, 0.0f)), "a step deeper into it is not");

			Tank* Third = CreateTankAt(InContext, "configs/enemies/Basic.json", Center - cocos2d::Vec2(TileSize, 0.0f));
			InContext.Expect(Third && !Third->CanMoveTo(Center - cocos2d::Vec2(TileSize - 16.0f, 0.0f)), "a free tank still cannot drive into another");
		});

		InSuite.Add("Movement: tanks inside each other separate by themselves, but not into a wall", [](TestContext& InContext)
		{
			const ResourceHolder CurrentTileset({ ResourceManifest().GetTilesetAtlas("field") });
			const cocos2d::Vec2 Center(TileSize * 4.5f, TileSize * 4.5f);
			Tank* First = CreateTankAt(InContext, PlayerTankPath(), Center);
			Tank* Second = CreateTankAt(InContext, PlayerTankPath(), Center);
			Wall* CurrentWall = CreateWall(InContext, "configs/wall_stone.json", Center + cocos2d::Vec2(TileSize, 0.0f));
			if ( !First || !Second || !CurrentWall )
			{
				return;
			}
			InContext.Expect(OverlapArea(*First, *Second) > 0.0f, "they start on the same spot");
			Advance(1.0f);
			InContext.ExpectEqual(OverlapArea(*First, *Second), 0.0f, "they no longer overlap");
			InContext.Expect(!CurrentWall->Blocks(First->GetCollisionRect()) && !CurrentWall->Blocks(Second->GetCollisionRect()), "none was pushed into the wall");
		});

		InSuite.Add("Movement: turning never moves the tank sideways, not even towards the grid", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateTankAt(InContext, PlayerTankPath(), cocos2d::Vec2(TileSize * 4.5f + 20.0f, TileSize * 4.5f));
			if ( !CurrentTank )
			{
				return;
			}
			CurrentTank->OnCommandReceived(GameCommand(CommandType::MoveRight));
			Advance(0.3f);
			const float StartX = CurrentTank->GetVisualNode()->getPositionX();

			CurrentTank->OnCommandReceived(GameCommand(CommandType::MoveForward));
			Advance(1.0f);
			InContext.ExpectNear(CurrentTank->GetVisualNode()->getPositionX(), StartX, 0.01f, "the same column after the turn and a second of driving");
			InContext.Expect(CurrentTank->GetVisualNode()->getPositionY() > TileSize * 5.5f, "it drove ahead");
			CurrentTank->OnCommandReceived(GameCommand(CommandType::Stop));
		});

		// Stone at columns 3 and 5 of row 6 leaves a one-tile gap at column 4; the tank comes from below.
		auto GapTest = [](TestContext& InContext, const float InOffset) -> float
		{
			const ResourceHolder CurrentTileset({ ResourceManifest().GetTilesetAtlas("field") });
			CreateWall(InContext, "configs/wall_stone.json", cocos2d::Vec2(TileSize * 3.5f, TileSize * 6.5f));
			CreateWall(InContext, "configs/wall_stone.json", cocos2d::Vec2(TileSize * 5.5f, TileSize * 6.5f));
			Tank* CurrentTank = CreateTankAt(InContext, PlayerTankPath(), cocos2d::Vec2(TileSize * 4.5f + InOffset, TileSize * 4.5f));
			if ( !CurrentTank )
			{
				return 0.0f;
			}
			CurrentTank->OnCommandReceived(GameCommand(CommandType::MoveForward));
			Advance(1.5f);
			CurrentTank->OnCommandReceived(GameCommand(CommandType::Stop));
			return CurrentTank->GetVisualNode()->getPositionY();
		};

		InSuite.Add("Movement: a tank catching the corner of a gap slides into it", [GapTest](TestContext& InContext)
		{
			const float Y = GapTest(InContext, 14.0f);
			InContext.Expect(Y > TileSize * 7.5f, "it got through the one-tile gap (y " + std::to_string(static_cast<int>(Y)) + ")");
		});

		InSuite.Add("Movement: a tank mostly against a wall does not slide, it stops", [GapTest](TestContext& InContext)
		{
			const float Y = GapTest(InContext, 30.0f);
			InContext.Expect(Y < TileSize * 6.0f, "it stopped below the wall (y " + std::to_string(static_cast<int>(Y)) + ")");
		});

		InSuite.Add("Spawning: an enemy never appears inside another one", [](TestContext& InContext)
		{
			LevelData CurrentLevel = LevelLoader::CreateEmpty();
			CurrentLevel.Rows[GridRows - 1][0] = tiles::PlayerSpawn;
			CurrentLevel.Rows[0][0] = tiles::EnemySpawn;
			CurrentLevel.EnemyIds = { "Heavy" };
			Wave CurrentWave;
			CurrentWave.Delay = 0.0f;
			CurrentWave.Interval = 0.0f;
			CurrentWave.MaxAlive = 3;
			CurrentWave.Entries.push_back({ tiles::EnemySpawn, 3 });
			CurrentLevel.Waves.push_back(CurrentWave);

			const cocos2d::Vec2 Point(TileSize / 2, (GridRows - 0.5f) * TileSize);
			EnemySpawner CurrentSpawner(InContext.GetSandbox(), cocos2d::Rect(0, 0, GridColumns * TileSize, GridRows * TileSize), CurrentLevel, { { tiles::EnemySpawn, { Point } } });
			std::vector<Tank*> CurrentEnemies;
			CurrentSpawner.OnSpawned = [&CurrentEnemies](Tank& InTank) { CurrentEnemies.push_back(&InTank); };
			for ( int CurrentFrame = 0; CurrentFrame < 60; ++CurrentFrame )
			{
				CurrentSpawner.Update(FrameTime);
				Advance(FrameTime);
			}

			for ( size_t Index = 0; Index < CurrentEnemies.size(); ++Index )
			{
				for ( size_t InnerIndex = Index + 1; InnerIndex < CurrentEnemies.size(); ++InnerIndex )
				{
					InContext.Expect(OverlapArea(*CurrentEnemies[Index], *CurrentEnemies[InnerIndex]) <= 0.0f, "enemies " + std::to_string(Index) + " and " + std::to_string(InnerIndex) + " do not overlap");
				}
			}
			InContext.Expect(!CurrentEnemies.empty(), "an enemy appeared");
			InContext.Note(std::to_string(CurrentEnemies.size()) + " enemies appeared in one second from one point");
			// Spawn flashes still running would call the spawner after this test.
			InContext.GetSandbox()->stopAllActions();
		});

		InSuite.Add("Display: window size options and the window size for a screen", [](TestContext& InContext)
		{
			InContext.ExpectEqual(NextWindowScale(100), 150, "100 -> 150");
			InContext.ExpectEqual(NextWindowScale(200), 0, "200 -> auto");
			InContext.ExpectEqual(NextWindowScale(0), 100, "auto -> 100");
			const PixelSize Design = { 1024, 864 };
			const PixelSize Screen = { 1920, 1040 };
			const PixelSize Normal = ComputeWindowSize(100, Design, Screen);
			InContext.Expect(Normal.Width == 1024 && Normal.Height == 864, "100% is the design size");
			const PixelSize Big = ComputeWindowSize(200, Design, Screen);
			InContext.Expect(Big.Height <= Screen.Height && Big.Width * 864 / 1024 == Big.Height, "200% is cut to the screen, the proportions kept");
			const PixelSize Automatic = ComputeWindowSize(0, Design, Screen);
			InContext.Expect(Automatic.Height < Screen.Height && Automatic.Height > 864, "auto fills most of the screen");
		});

		InSuite.Add("Display: the display settings survive saving and loading the profile", [](TestContext& InContext)
		{
			auto& Manager = GameContext::Get().GetManager<ProfileManager>();
			const PlayerProfile Saved = Manager.Get();
			auto& CurrentProfile = Manager.Get();
			CurrentProfile.bVerticalSync = false;
			CurrentProfile.CurrentWindowMode = WindowMode::Borderless;
			CurrentProfile.WindowScale = 150;
			CurrentProfile.bSmoothPixels = true;
			CurrentProfile.EffectsVolume = 40;
			CurrentProfile.bHdTextures = false;
			CurrentProfile.WindowPosition = PixelPoint{ -1200, 35 };
			Manager.Save();
			CurrentProfile = PlayerProfile();
			Manager.Load();
			InContext.Expect(!Manager.Get().bVerticalSync, "vertical sync");
			InContext.Expect(Manager.Get().CurrentWindowMode == WindowMode::Borderless, "window mode");
			InContext.ExpectEqual(Manager.Get().WindowScale, 150, "window size");
			InContext.Expect(Manager.Get().bSmoothPixels, "pixel smoothing");
			InContext.ExpectEqual(Manager.Get().EffectsVolume, 40, "effects volume");
			InContext.Expect(!Manager.Get().bHdTextures, "SD textures");
			InContext.Expect(Manager.Get().WindowPosition.has_value(), "the window position is kept");
			if ( Manager.Get().WindowPosition )
			{
				InContext.ExpectEqual(Manager.Get().WindowPosition->X, -1200, "window x (a screen on the left)");
				InContext.ExpectEqual(Manager.Get().WindowPosition->Y, 35, "window y");
			}
			Manager.Get().WindowPosition.reset();
			Manager.Save();
			Manager.Load();
			InContext.Expect(!Manager.Get().WindowPosition.has_value(), "no saved position means centred");
			Manager.Get() = Saved;
			Manager.Save();
		});

		InSuite.Add("Display: a saved window position is kept on the screen", [](TestContext& InContext)
		{
			const PixelRect Work = { 0, 0, 1920, 1040 };
			const PixelSize Window = { 1040, 900 };
			const PixelPoint Inside = KeepWindowInArea({ 100, 50 }, Window, Work);
			InContext.Expect(Inside.X == 100 && Inside.Y == 50, "a place inside the screen is kept");
			const PixelPoint Right = KeepWindowInArea({ 1500, 600 }, Window, Work);
			InContext.Expect(Right.X == 880 && Right.Y == 140, "a window out over the edge is moved back in");
			const PixelPoint Left = KeepWindowInArea({ -300, -20 }, Window, Work);
			InContext.Expect(Left.X == 0 && Left.Y == 0, "and over the left and top edges");
			const PixelPoint Second = KeepWindowInArea({ -1800, 100 }, Window, { -1920, 0, 0, 1040 });
			InContext.Expect(Second.X == -1800 && Second.Y == 100, "a screen on the left (negative coordinates)");
			const PixelPoint Huge = KeepWindowInArea({ 400, 300 }, { 2500, 1200 }, Work);
			InContext.Expect(Huge.X == 0 && Huge.Y == 0, "a window bigger than the screen keeps its corner on it");
		});

		InSuite.Add("Display: HD textures map to their folder", [](TestContext& InContext)
		{
			const TextureQualityConfig& Config = GetTextureQualityConfig();
			InContext.Expect(Config.HdScale > 1.0f, "the HD set is bigger");
			InContext.ExpectEqual(MapAtlasPath("textures/player.plist", false, Config), std::string("textures/player.plist"), "SD keeps the path");
			InContext.ExpectEqual(MapAtlasPath("textures/enemies/Basic.plist", true, Config), Config.HdFolder + "enemies/Basic.plist", "HD maps the path");
			InContext.ExpectEqual(MapAtlasPath("other/x.plist", true, Config), std::string("other/x.plist"), "a path outside the SD folder is kept");
			for ( const std::string& Atlas : ResourceManifest().GetGlobalAtlases() )
			{
				InContext.Expect(cocos2d::FileUtils::getInstance()->isFileExist(MapAtlasPath(PathOf(model::ResourceId(Atlas), model::ResourceKind::Atlas), true, Config)), "an HD atlas for " + Atlas);
			}
		});

		InSuite.Add("Display: switching HD and SD reloads the atlases, sprites keep their size", [](TestContext& InContext)
		{
			const bool bWasHd = IsHdTextures();
			const std::string Atlas = "atlas.player";
			const std::string AtlasFile = "textures/player.plist";
			const std::string Frame = "player_tank";

			ApplyHdTextures(false);
			InContext.ExpectNear(cocos2d::Director::getInstance()->getContentScaleFactor(), 1.0f, 0.001f, "SD: content scale 1");
			InContext.ExpectEqual(GameContext::Get().GetManager<ResourceManager>().GetAtlasLoadedPath(Atlas), AtlasFile, "SD atlas loaded");
			cocos2d::SpriteFrame* SdFrame = cocos2d::SpriteFrameCache::getInstance()->getSpriteFrameByName(Frame);
			InContext.Expect(SdFrame != nullptr, "SD frame");
			const cocos2d::Size SdSize = SdFrame ? SdFrame->getOriginalSize() : cocos2d::Size::ZERO;

			ApplyHdTextures(true);
			const TextureQualityConfig& Config = GetTextureQualityConfig();
			InContext.ExpectNear(cocos2d::Director::getInstance()->getContentScaleFactor(), Config.HdScale, 0.001f, "HD: content scale raised");
			InContext.ExpectEqual(GameContext::Get().GetManager<ResourceManager>().GetAtlasLoadedPath(Atlas), MapAtlasPath(AtlasFile, true, Config), "HD atlas loaded");
			cocos2d::SpriteFrame* HdFrame = cocos2d::SpriteFrameCache::getInstance()->getSpriteFrameByName(Frame);
			InContext.Expect(HdFrame != nullptr, "HD frame");
			if ( HdFrame )
			{
				InContext.ExpectNear(HdFrame->getOriginalSize().width, SdSize.width, 0.5f, "same width in points");
				InContext.ExpectNear(HdFrame->getOriginalSize().height, SdSize.height, 0.5f, "same height in points");
				InContext.ExpectNear(HdFrame->getOriginalSizeInPixels().width, SdSize.width * Config.HdScale, 0.5f, "more pixels");
			}
			ApplyHdTextures(bWasHd);
		});

		InSuite.Add("Display: switching SD, HD and SD keeps label font atlases valid", [](TestContext& InContext)
		{
			const bool bWasHd = IsHdTextures();
			ApplyHdTextures(false);
			constexpr float DesignFontSize = 24.0f;
			cocos2d::TTFConfig FontConfig(PathOf(ids::fonts::Plain, model::ResourceKind::Font).c_str(), DesignFontSize * view::cocos::widgets::GetUiScale());
			cocos2d::Label* MenuLabel = view::cocos::widgets::CreateLabel("Continue", DesignFontSize, cocos2d::Color3B::WHITE, false);
			cocos2d::Director::getInstance()->getRunningScene()->addChild(MenuLabel);
			const cocos2d::Size SdSize = MenuLabel->getContentSize();
			float SdLineHeight = 0.0f;

			const auto CheckAtlas = [&](const std::string_view InStep)
			{
				const float Scale = cocos2d::Director::getInstance()->getContentScaleFactor();
				// The cache lends the atlas without a reference: releasing it here would free it under the label.
				cocos2d::FontAtlas* Atlas = cocos2d::FontAtlasCache::getFontAtlasTTF(&FontConfig);
				InContext.Expect(Atlas != nullptr && Atlas->getTexture(0) != nullptr, std::string(InStep) + ": atlas has a texture");
				InContext.Expect(MenuLabel->getFontAtlas() == Atlas, std::string(InStep) + ": the label uses the cached atlas");
				if ( Atlas )
				{
					const float LineHeightInPoints = Atlas->getLineHeight() / Scale;
					if ( SdLineHeight == 0.0f )
					{
						SdLineHeight = LineHeightInPoints;
					}
					InContext.ExpectNear(LineHeightInPoints, SdLineHeight, 1.5f, std::string(InStep) + ": glyphs rasterized at the content scale");
				}
				MenuLabel->setString("Continue!");
				MenuLabel->setString("Continue");
				MenuLabel->updateContent();
				InContext.ExpectNear(MenuLabel->getContentSize().width, SdSize.width, 2.0f, std::string(InStep) + ": label keeps its width");
				InContext.ExpectNear(MenuLabel->getContentSize().height, SdSize.height, 2.0f, std::string(InStep) + ": label keeps its height");
			};

			CheckAtlas("SD");
			ApplyHdTextures(true);
			CheckAtlas("HD");
			ApplyHdTextures(false);
			CheckAtlas("SD again");
			MenuLabel->removeFromParent();
			ApplyHdTextures(bWasHd);
		});

		InSuite.Add("Display: smoothing, vertical sync and the volume apply at once", [](TestContext& InContext)
		{
			GameContext::Get().GetManager<ResourceManager>().SetSmoothPixels(true);
			InContext.Expect(GameContext::Get().GetManager<ResourceManager>().IsSmoothPixels(), "smooth pixels");
			GameContext::Get().GetManager<ResourceManager>().SetSmoothPixels(false);
			InContext.Expect(!GameContext::Get().GetManager<ResourceManager>().IsSmoothPixels(), "sharp pixels again");

			const bool bVsync = IsVerticalSyncEnabled();
			SetVerticalSyncEnabled(true);
			InContext.Expect(GetVerticalSyncInterval() != 0, "the driver has vertical sync on");
			SetVerticalSyncEnabled(false);
			InContext.Expect(GetVerticalSyncInterval() <= 0, "and off");
			SetVerticalSyncEnabled(bVsync);

			const float CurrentVolume = GameContext::Get().GetManager<AudioManager>().GetVolume();
			GameContext::Get().GetManager<AudioManager>().SetVolume(0.4f);
			InContext.ExpectNear(GameContext::Get().GetManager<AudioManager>().GetVolume(), 0.4f, 0.001f, "the effects volume");
			GameContext::Get().GetManager<AudioManager>().SetVolume(CurrentVolume);
		});

		InSuite.Add("Display: a level's frame rate limit is applied, menus run at 60", [](TestContext& InContext)
		{
			auto DirectorInstance = cocos2d::Director::getInstance();
			ApplyFpsLimit(30);
			InContext.ExpectNear(static_cast<float>(DirectorInstance->getAnimationInterval()), 1.0f / 30.0f, 0.0001f, "30 FPS");
			ApplyFpsLimit(0);
			InContext.Expect(DirectorInstance->getAnimationInterval() < 1.0 / 500.0, "unlimited");
			ApplyMenuFrameRate();
			InContext.ExpectNear(static_cast<float>(DirectorInstance->getAnimationInterval()), 1.0f / MenuFps, 0.0001f, "the menu rate");
		});
	}

	void AddAiTests(TestSuite& InSuite)
	{
		InSuite.Add("AI: tank AI profiles come from configs/ai/tank_ai.json", [](TestContext& InContext)
		{
			const TankAiSettings Wander = AiSettingsLoader::LoadTankAi(AiType::Wander);
			const TankAiSettings Hunter = AiSettingsLoader::LoadTankAi(AiType::Hunter);
			InContext.ExpectNear(Wander.GetThinkIntervalMin(), 0.8f, 0.001f, "wander think interval min");
			InContext.ExpectNear(Wander.GetThinkIntervalMax(), 2.2f, 0.001f, "wander think interval max");
			InContext.ExpectNear(Wander.GetFocusOnPlayerChance(), 0.0f, 0.001f, "a wanderer never heads for the player");
			InContext.ExpectNear(Hunter.GetThinkIntervalMin(), 0.5f, 0.001f, "hunter think interval min");
			InContext.ExpectNear(Hunter.GetFocusOnPlayerChance(), 0.7f, 0.001f, "hunter focus");
			InContext.ExpectNear(Hunter.GetAimTolerance(), 0.45f * TileSize, 0.01f, "aim tolerance is given in tiles");
			InContext.Expect(Hunter.GetRandomFireRate() > 0.0f && Hunter.GetMaxTurnWait() > 0.0f && Hunter.GetCellCenterTolerance() > 0.0f,
				"every key of the profile is read");
		});

		InSuite.Add("AI: player bot settings come from configs/ai/player_bot.json", [](TestContext& InContext)
		{
			const PlayerBotSettings Settings = AiSettingsLoader::LoadPlayerBot();
			InContext.ExpectNear(Settings.GetThinkInterval(), 0.25f, 0.001f, "think interval");
			InContext.ExpectNear(Settings.GetHoldDistance(), 7.0f * TileSize, 0.01f, "hold distance in tiles");
			InContext.ExpectNear(Settings.GetHomeHeight(), 3.0f * TileSize, 0.01f, "home rows in tiles");
			InContext.ExpectNear(Settings.GetFacingAlignment(), 0.9f, 0.001f, "facing alignment");
			InContext.Expect(Settings.GetStuckTime() > 0.0f && Settings.GetAbilityRange() > 0.0f && Settings.GetAimTolerance() > 0.0f,
				"every key is read");
		});

		InSuite.Add("AI: settings keep their values in range", [](TestContext& InContext)
		{
			TankAiSettings Settings;
			Settings.SetThinkInterval(2.0f, 1.0f);
			InContext.ExpectNear(Settings.GetThinkIntervalMin(), 1.0f, 0.001f, "a swapped interval is ordered");
			InContext.ExpectNear(Settings.GetThinkIntervalMax(), 2.0f, 0.001f, "a swapped interval is ordered (max)");
			Settings.SetFocusOnPlayerChance(1.5f);
			InContext.ExpectNear(Settings.GetFocusOnPlayerChance(), 1.0f, 0.001f, "a chance is at most 1");
			Settings.SetRandomFireRate(-1.0f);
			InContext.ExpectNear(Settings.GetRandomFireRate(), 0.0f, 0.001f, "a rate is never negative");

			const auto Partial = JsonConfigNode::Parse(R"({"think_interval":[0.1,0.3]})", "test");
			const TankAiSettings FromPartial = AiSettingsLoader::ReadTankAi(*Partial);
			InContext.ExpectNear(FromPartial.GetThinkIntervalMax(), 0.3f, 0.001f, "present keys are read");
			InContext.ExpectNear(FromPartial.GetAimTolerance(), 0.0f, 0.001f, "missing keys give 0");
		});

		InSuite.Add("AI: random moves are move commands only", [](TestContext& InContext)
		{
			bool bOnlyMoves = true;
			for ( int Index = 0; Index < 200; ++Index )
			{
				const CommandType Move = AiQueries::RandomMove();
				bOnlyMoves = bOnlyMoves && (Move == CommandType::MoveForward || Move == CommandType::MoveBack
					|| Move == CommandType::MoveLeft || Move == CommandType::MoveRight);
			}
			InContext.Expect(bOnlyMoves, "200 random moves");
		});

		InSuite.Add("AI: moves along the longer or the shorter axis", [](TestContext& InContext)
		{
			InContext.Expect(AiQueries::MoveAlongMajorAxis(cocos2d::Vec2(10.0f, 2.0f)) == CommandType::MoveRight, "major: right");
			InContext.Expect(AiQueries::MoveAlongMajorAxis(cocos2d::Vec2(-1.0f, -5.0f)) == CommandType::MoveBack, "major: down");
			InContext.Expect(AiQueries::MoveAlongMinorAxis(cocos2d::Vec2(10.0f, 2.0f)) == CommandType::MoveForward, "minor: up");
			InContext.Expect(AiQueries::MoveAlongMinorAxis(cocos2d::Vec2(-1.0f, -5.0f)) == CommandType::MoveLeft, "minor: left");
		});
	}

	void AddObjectCastTests(TestSuite& InSuite)
	{
		InSuite.Add("ObjectCast: casts by the object type, without dynamic_cast", [](TestContext& InContext)
		{
			const ResourceHolder CurrentTileset({ ResourceManifest().GetTilesetAtlas("field") });
			Tank* Player = CreateTank(InContext, PlayerTankPath());
			Wall* Brick = CreateWall(InContext, "configs/wall_brick.json", cocos2d::Vec2(TileSize / 2, TileSize / 2));
			if ( !Player || !Brick )
			{
				return;
			}
			GameObject* PlayerObject = Player;
			GameObject* WallObject = Brick;
			const GameObject* ConstWallObject = Brick;
			InContext.Expect(ObjectCast<Tank>(PlayerObject) == Player, "a tank is a tank");
			InContext.Expect(ObjectCast<MovableObject>(PlayerObject) != nullptr, "a tank is movable");
			InContext.Expect(ObjectCast<PhysicsObject>(WallObject) != nullptr, "a wall is a physics object");
			InContext.Expect(ObjectCast<Tank>(WallObject) == nullptr, "a wall is not a tank");
			InContext.Expect(ObjectCast<MovableObject>(ConstWallObject) == nullptr, "a wall is not movable (const)");
			InContext.Expect(ObjectCast<Wall>(static_cast<GameObject*>(nullptr)) == nullptr, "nullptr stays nullptr");
		});

		InSuite.Add("SceneCast: casts the running scene by its kind tag", [](TestContext& InContext)
		{
			cocos2d::Scene* Settings = view::cocos::SettingsScene::create();
			cocos2d::Scene* Plain = cocos2d::Scene::create();
			InContext.Expect(view::cocos::SceneCast<view::cocos::SettingsScene>(Settings) == Settings, "a settings scene is a settings scene");
			InContext.Expect(view::cocos::SceneCast<view::cocos::MenuScene>(Settings) == nullptr, "a settings scene is not a menu scene");
			InContext.Expect(view::cocos::SceneCast<view::cocos::MenuScene>(Plain) == nullptr, "a plain (transition) scene is no game scene");
			InContext.Expect(view::cocos::SceneCast<view::cocos::MenuScene>(nullptr) == nullptr, "nullptr stays nullptr");
		});

		InSuite.Add("Button::FromNode: finds buttons without dynamic_cast", [](TestContext& InContext)
		{
			auto CurrentButton = view::cocos::widgets::Button::create("ok", cocos2d::Size(100.0f, 30.0f), []() {});
			auto PlainNode = cocos2d::Node::create();
			InContext.Expect(view::cocos::widgets::Button::FromNode(CurrentButton) == CurrentButton, "a button is found");
			InContext.Expect(view::cocos::widgets::Button::FromNode(PlainNode) == nullptr, "a plain node is not a button");
			InContext.Expect(view::cocos::widgets::Button::FromNode(nullptr) == nullptr, "nullptr stays nullptr");
		});

		InSuite.Add("ObjectsFactory: typed search looks only at the type asked for", [](TestContext& InContext)
		{
			Tank* Player = CreateTank(InContext, PlayerTankPath());
			if ( !Player )
			{
				return;
			}
			InContext.Expect(GameContext::Get().GetManager<ObjectsFactory>().FindObject<Tank>(PlayerName) != nullptr, "the player is found as a tank");
			InContext.Expect(GameContext::Get().GetManager<ObjectsFactory>().FindObject<Wall>(PlayerName) == nullptr, "the player is not found as a wall");
			int TankCount = 0;
			GameContext::Get().GetManager<ObjectsFactory>().ForEachObject<Tank>([&TankCount](Tank&) { ++TankCount; });
			InContext.Expect(TankCount >= 1, "ForEachObject<Tank> visits the tank");
		});
	}

	void AddMathTests(TestSuite& InSuite)
	{
		using model::Color;
		using model::RectF;
		using model::Vec2f;

		InSuite.Add("Vec2f: arithmetic, length, normalize", [](TestContext& InContext)
		{
			const Vec2f A(3.0f, 4.0f);
			InContext.Expect(A + Vec2f(1.0f, 1.0f) == Vec2f(4.0f, 5.0f), "addition");
			InContext.Expect(A - Vec2f(1.0f, 1.0f) == Vec2f(2.0f, 3.0f), "subtraction");
			InContext.Expect(A * 2.0f == Vec2f(6.0f, 8.0f) && 2.0f * A == Vec2f(6.0f, 8.0f), "scaling");
			InContext.Expect(-A == Vec2f(-3.0f, -4.0f), "negation");
			InContext.ExpectNear(A.Length(), 5.0f, 0.0001f, "length");
			InContext.ExpectNear(A.Dot(Vec2f(1.0f, 0.0f)), 3.0f, 0.0001f, "dot");
			InContext.ExpectNear(A.Normalized().Length(), 1.0f, 0.0001f, "normalized length");
			InContext.Expect(Vec2f().Normalized() == Vec2f(), "zero stays zero");
			InContext.ExpectNear(Vec2f().DistanceTo(A), 5.0f, 0.0001f, "distance");
		});

		InSuite.Add("RectF: bounds, contains, intersects", [](TestContext& InContext)
		{
			const RectF Box(0.0f, 0.0f, 64.0f, 32.0f);
			InContext.Expect(Box.GetMaxX() == 64.0f && Box.GetMaxY() == 32.0f, "max corner");
			InContext.Expect(Box.GetCenter() == Vec2f(32.0f, 16.0f), "center");
			InContext.Expect(RectF::FromCenter(Vec2f(32.0f, 16.0f), Vec2f(64.0f, 32.0f)) == Box, "from center");
			InContext.Expect(Box.Contains(Vec2f(64.0f, 32.0f)), "edges are inside");
			InContext.Expect(!Box.Contains(Vec2f(64.1f, 0.0f)), "outside point");
			InContext.Expect(Box.Intersects(RectF(63.0f, 31.0f, 10.0f, 10.0f)), "overlap");
			InContext.Expect(!Box.Intersects(RectF(64.0f, 0.0f, 10.0f, 10.0f)), "touching edges do not intersect");
		});

		InSuite.Add("JsonValue: parses objects, arrays and scalars without the engine", [](TestContext& InContext)
		{
			bool bOk = false;
			const model::JsonValue Root = model::JsonValue::Parse(R"({"name":"heavy","health":4,"speed":1.5,"boss":true,"spawners":[1,2,3],"ai":{"think":0.25}})", &bOk);
			InContext.Expect(bOk && Root.IsObject(), "object parsed");
			InContext.Expect(Root.GetString("name", "") == "heavy", "string");
			InContext.Expect(Root.GetInt("health", 0) == 4, "int");
			InContext.ExpectNear(Root.GetFloat("speed", 0.0f), 1.5f, 0.0001f, "float");
			InContext.ExpectNear(Root.GetFloat("health", 0.0f), 4.0f, 0.0001f, "an int reads as a float");
			InContext.Expect(Root.GetBool("boss", false), "bool");
			InContext.Expect(Root["spawners"].GetArray().size() == 3 && Root["spawners"].GetArray()[2].AsInt() == 3, "array");
			InContext.ExpectNear(Root["ai"].GetFloat("think", 0.0f), 0.25f, 0.0001f, "nested object");
		});

		InSuite.Add("JsonValue: fallbacks for missing keys, wrong types and bad text", [](TestContext& InContext)
		{
			const model::JsonValue Root = model::JsonValue::Parse(R"({"health":"many"})");
			InContext.Expect(Root.GetInt("health", 7) == 7, "wrong type gives the fallback");
			InContext.Expect(Root.GetInt("armor", 2) == 2, "missing key gives the fallback");
			InContext.Expect(!Root.Has("armor") && Root["armor"].IsNull(), "missing key reads as null");
			InContext.Expect(Root["health"]["deeper"].IsNull(), "a lookup into a non-object is null");
			bool bOk = true;
			const model::JsonValue Broken = model::JsonValue::Parse("{\"health\": ", &bOk);
			InContext.Expect(!bOk && Broken.IsNull(), "a syntax error is reported");
		});

		InSuite.Add("IConfigNode: typed reads, defaults, Vec2f, Color and arrays", [](TestContext& InContext)
		{
			bool bOk = false;
			const auto Root = JsonConfigNode::Parse(R"({"speed":1.5,"lives":3,"boss":true,"name":"heavy","spawn":[2,5],"tint":[10,20,30],"bad_tint":[1,2],"waves":[{"count":4},{"count":6}]})", "test", &bOk);
			InContext.Expect(bOk, "parsed");
			InContext.ExpectNear(Root->GetValue<float>("speed"), 1.5f, 0.0001f, "float");
			InContext.Expect(Root->GetValue<int>("lives") == 3 && Root->GetValue<bool>("boss"), "int and bool");
			InContext.Expect(Root->GetValue<std::string>("name") == "heavy", "string");
			InContext.Expect(Root->GetValue<model::Vec2f>("spawn") == model::Vec2f(2.0f, 5.0f), "Vec2f from [x, y]");
			InContext.Expect(Root->GetValue<model::Color>("tint") == model::Color(10, 20, 30, 255), "Color from [r, g, b]");
			model::Color Tint(1, 1, 1);
			InContext.Expect(!Root->TryRead("bad_tint", Tint) && Tint == model::Color(1, 1, 1), "a short color is rejected and Out stays");
			InContext.Expect(Root->GetValueOr("armor", 2) == 2 && Root->GetValueOr("name", "x") == "heavy", "GetValueOr");
			InContext.Expect(Root->GetValue<int, false>("speed") == 1, "a float reads as int");
			InContext.Expect(Root->GetValue<int, false>("name") == 0, "a wrong type gives zero");
			const model::IConfigNode* const Waves = Root->FindChild("waves");
			InContext.Expect(Waves && Waves->GetItemCount() == 2 && Waves->GetItem(1)->GetValue<int>("count") == 6, "array of objects");
			InContext.Expect(!Root->Has("missing") && !Waves->GetItem(5), "missing key and index");
		});

		InSuite.Add("IConfigNode: vectors, maps and types that read themselves", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(R"({"rewards":[1,2,3],"bad":[1,"x"],"costs":{"armor":20,"speed":15},"waves":[{"count":4,"delay":1.5},{"count":6}]})", "test");
			InContext.Expect(Root->GetValue<std::vector<int>>("rewards") == std::vector<int>{ 1, 2, 3 }, "vector of ints");
			std::vector<int> Untouched{ 9 };
			InContext.Expect(!Root->TryRead("bad", Untouched) && Untouched == std::vector<int>{ 9 }, "a bad item rejects the whole vector");
			const auto Costs = Root->GetValue<std::map<std::string, int>>("costs");
			InContext.Expect(Costs.size() == 2 && Costs.at("armor") == 20 && Costs.at("speed") == 15, "map by key");
			const auto Waves = Root->GetValue<std::vector<TestWaveDef>>("waves");
			InContext.Expect(Waves.size() == 2 && Waves[0].Count == 4 && Waves[1].Count == 6, "custom type via FromConfig");
			InContext.ExpectNear(Waves[0].Delay, 1.5f, 0.0001f, "custom type reads its own keys");
			InContext.ExpectNear(Waves[1].Delay, 0.0f, 0.0001f, "custom type default for a missing key");
			InContext.Expect(Root->GetInt("missing") == 0 && Root->GetString("missing").empty(), "short forms");
		});

		InSuite.Add("TestSelection: --test modes pick their modules", [](TestContext& InContext)
		{
			const TestSelection None = TestSelection::Parse("TanksClone.exe --showcase out");
			InContext.Expect(None.IsEmpty() && None.bValid, "no --test: nothing selected");

			const TestSelection Plain = TestSelection::Parse("TanksClone.exe --test --background");
			InContext.Expect(Plain.bUnit && Plain.bFields && !Plain.bCampaign && !Plain.bBenchmark && !Plain.bMenu, "plain --test is short");

			const TestSelection Menu = TestSelection::Parse("TanksClone.exe --test=menu");
			InContext.Expect(Menu.bMenu && !Menu.bUnit && !Menu.HasLevelCases(), "menu only");

			const TestSelection Benchmark = TestSelection::Parse("TanksClone.exe --test=benchmark");
			InContext.Expect(Benchmark.bBenchmark && !Benchmark.bFields && !Benchmark.bCampaign && !Benchmark.bUnit, "benchmark only");

			const TestSelection Campaign = TestSelection::Parse("--background --test=campaign");
			InContext.Expect(Campaign.bCampaign && !Campaign.bFields && !Campaign.bMenu, "campaign only");

			const TestSelection Full = TestSelection::Parse("TanksClone.exe --test=full");
			InContext.Expect(Full.bUnit && Full.bFields && Full.bCampaign && Full.bBenchmark && Full.bMenu && Full.bCrash, "full has everything");

			const TestSelection Crash = TestSelection::Parse("TanksClone.exe --test=crash --background");
			InContext.Expect(Crash.bCrash && !Crash.bUnit && !Crash.HasLevelCases() && !Crash.IsEmpty(), "crash only");

			const TestSelection Unknown = TestSelection::Parse("TanksClone.exe --test=nonsense");
			InContext.Expect(!Unknown.bValid && Unknown.bUnit && Unknown.bFields && !Unknown.bMenu, "unknown mode: short, marked invalid");

			InContext.Expect(TestSelection::Parse("TanksClone.exe --testing").IsEmpty(), "a longer word is not --test");
		});

		InSuite.Add("Random: the same seed gives the same sequence, ranges are inclusive", [](TestContext& InContext)
		{
			model::Random First(42);
			model::Random Second(42);
			bool bSame = true;
			bool bInRange = true;
			bool bHitMin = false;
			bool bHitMax = false;
			for ( int Index = 0; Index < 1000; ++Index )
			{
				const int Value = First.Int(-2, 3);
				bSame = bSame && Value == Second.Int(-2, 3);
				bInRange = bInRange && Value >= -2 && Value <= 3;
				bHitMin = bHitMin || Value == -2;
				bHitMax = bHitMax || Value == 3;
				const float Fraction = First.Float(0.5f, 1.5f);
				bSame = bSame && Fraction == Second.Float(0.5f, 1.5f);
				bInRange = bInRange && Fraction >= 0.5f && Fraction <= 1.5f;
			}
			InContext.Expect(bSame, "two generators with one seed agree");
			InContext.Expect(bInRange && bHitMin && bHitMax, "Int covers both bounds and nothing outside");
			First.SetSeed(42);
			model::Random Fresh(42);
			InContext.Expect(First.NextUInt32() == Fresh.NextUInt32(), "SetSeed restarts the sequence");
			InContext.Expect(model::Random(1).NextUInt32() != model::Random(2).NextUInt32(), "different seeds differ");
			InContext.Expect(First.Chance(1.0f) && !First.Chance(0.0f), "chance 1 always, chance 0 never");
		});

		InSuite.Add("CocosConvert: model types round-trip through cocos2d types", [](TestContext& InContext)
		{
			const Vec2f Point(12.5f, -3.0f);
			InContext.Expect(view::cocos::FromCocos(view::cocos::ToCocos(Point)) == Point, "Vec2f");
			const RectF Box(1.0f, 2.0f, 3.0f, 4.0f);
			InContext.Expect(view::cocos::FromCocos(view::cocos::ToCocos(Box)) == Box, "RectF");
			const cocos2d::Rect CocosBox = view::cocos::ToCocos(Box);
			InContext.Expect(CocosBox.containsPoint(cocos2d::Vec2(4.0f, 6.0f)) == Box.Contains(Vec2f(4.0f, 6.0f)), "same edge rule as cocos2d");
			const Color Tint(10, 20, 30, 40);
			InContext.Expect(view::cocos::FromCocos(view::cocos::ToCocos(Tint)) == Tint, "Color");
		});
	}

	bool HasOverlap(const std::vector<model::RectF>& InRects)
	{
		for ( std::size_t First = 0; First < InRects.size(); ++First )
		{
			for ( std::size_t Second = First + 1; Second < InRects.size(); ++Second )
			{
				if ( InRects[First].Intersects(InRects[Second]) )
				{
					return true;
				}
			}
		}
		return false;
	}

	bool IsInside(const model::RectF& InOuter, const model::RectF& InInner)
	{
		return InInner.GetMinX() >= InOuter.GetMinX() && InInner.GetMinY() >= InOuter.GetMinY()
			&& InInner.GetMaxX() <= InOuter.GetMaxX() && InInner.GetMaxY() <= InOuter.GetMaxY();
	}

	void AddEditorTests(TestSuite& InSuite)
	{
		using model::EditorLayout;
		using model::EditorLayoutResult;
		using model::RectF;

		InSuite.Add("EditorLayout: panels and buttons never overlap at any toolbar size", [](TestContext& InContext)
		{
			const std::vector<RectF> Toolbars = { RectF(0, 704, 1024, 160), RectF(0, 704, 1400, 160), RectF(0, 704, 800, 160),
				RectF(-120, 704, 1264, 220), RectF(0, 704, 1024, 100) };
			for ( const RectF& Bounds : Toolbars )
			{
				for ( std::size_t Items = 0; Items <= 12; ++Items )
				{
					const EditorLayoutResult Layout = EditorLayout(4, 4).Compute(Bounds, Items);
					const std::string Where = "width " + std::to_string(static_cast<int>(Bounds.Size.X)) + ", items " + std::to_string(Items);
					std::vector<RectF> All = Layout.Tabs;
					All.insert(All.end(), Layout.Items.begin(), Layout.Items.end());
					All.insert(All.end(), Layout.Actions.begin(), Layout.Actions.end());
					All.push_back(Layout.CounterMinus);
					All.push_back(Layout.CounterLabel);
					All.push_back(Layout.CounterPlus);
					InContext.Expect(!HasOverlap(All), "no overlaps, " + Where);
					InContext.Expect(!Layout.PalettePanel.Intersects(Layout.ActionPanel), "panels apart, " + Where);
					InContext.Expect(IsInside(Bounds, Layout.PalettePanel) && IsInside(Bounds, Layout.ActionPanel), "panels inside, " + Where);
					InContext.Expect(Layout.bItemsFit, "items fit, " + Where);
					for ( const RectF& Rect : Layout.Tabs )
					{
						InContext.Expect(IsInside(Layout.PalettePanel, Rect), "tab in the palette panel, " + Where);
					}
					for ( const RectF& Rect : Layout.Items )
					{
						InContext.Expect(IsInside(Layout.PalettePanel, Rect), "item in the palette panel, " + Where);
					}
					for ( const RectF& Rect : Layout.Actions )
					{
						InContext.Expect(IsInside(Layout.ActionPanel, Rect), "action in the action panel, " + Where);
					}
					InContext.Expect(IsInside(Layout.ActionPanel, Layout.CounterMinus) && IsInside(Layout.ActionPanel, Layout.CounterPlus),
						"counter in the action panel, " + Where);
				}
			}
		});

		InSuite.Add("EditorLayout: one button per tab, action and item", [](TestContext& InContext)
		{
			const EditorLayoutResult Layout = EditorLayout(4, 3).Compute(RectF(0, 704, 1024, 160), 5);
			InContext.Expect(Layout.Tabs.size() == 3, "tabs");
			InContext.Expect(Layout.Actions.size() == 4, "actions");
			InContext.Expect(Layout.Items.size() == 5, "items");
			InContext.Expect(Layout.Items[0].Size.X == Layout.Items[0].Size.Y, "items are square");
		});

		InSuite.Add("EditorLayout: more items shrink, too many report that they do not fit", [](TestContext& InContext)
		{
			const RectF Bounds(0, 704, 1024, 160);
			const float Few = EditorLayout(4, 4).Compute(Bounds, 3).Items[0].Size.X;
			const float Many = EditorLayout(4, 4).Compute(Bounds, 30).Items[0].Size.X;
			InContext.Expect(Many < Few, "more items, smaller buttons");
			InContext.Expect(!EditorLayout(4, 4).Compute(Bounds, 5000).bItemsFit, "5000 items do not fit");
		});

		InSuite.Add("EditorPalette: tabs and entries come from the config and the object configs", [](TestContext& InContext)
		{
			const auto Wall = JsonConfigNode::Parse(R"({"object_type":"Wall","texture_path":"wall_x","scale":2.0})", "wall");
			const auto Bonus = JsonConfigNode::Parse(R"({"object_type":"Bonus","texture_path":"bonus_x","scale":2.5})", "bonus");
			const auto Root = JsonConfigNode::Parse(R"({"tabs":[{"id":"walls","name":"T_W"},{"id":"tools"}],
				"entries":[{"tab":"walls","tile":"X","name":"N_X","config":"wall"},
				{"tab":"walls","tile":"B","config":"bonus"},
				{"tab":"tools","tile":".","icon":"erase"},
				{"tab":"units","tile":"P","frame":"p"},
				{"tab":"walls","tile":"QQ","frame":"q"},
				{"tab":"walls","tile":"M","config":"missing"}]})", "palette");
			const auto Find = [&Wall, &Bonus](const std::string& InId) -> const model::IConfigNode*
			{
				return InId == "wall" ? Wall.get() : (InId == "bonus" ? Bonus.get() : nullptr);
			};

			model::EditorPalette Palette;
			InContext.Expect(Palette.FromConfig(*Root, Find), "palette read");
			InContext.Expect(Palette.GetTabs().size() == 2, "tabs");
			InContext.Expect(Palette.GetTabs()[1].NameKey == "tools", "a tab without a name shows its id");
			InContext.Expect(Palette.GetEntries().size() == 3, "valid entries");
			InContext.Expect(Palette.GetErrors().size() == 3, "skipped entries are reported");
			InContext.Expect(Palette.GetEntriesOfTab(0).size() == 2, "entries on the walls tab");

			const model::EditorPaletteEntry* const WallEntry = Palette.FindByTile('X');
			InContext.Expect(WallEntry && WallEntry->Frame == "wall_x" && WallEntry->Icon == model::PaletteIcon::TilesetFrame
				&& WallEntry->MapScale == 2.0f, "a wall takes its frame and scale from its config");
			const model::EditorPaletteEntry* const BonusEntry = Palette.FindByTile('B');
			InContext.Expect(BonusEntry && BonusEntry->Icon == model::PaletteIcon::Frame && BonusEntry->MapScale == 2.5f, "a bonus uses a shared frame");
			const model::EditorPaletteEntry* const Eraser = Palette.FindByTile('.');
			InContext.Expect(Eraser && Eraser->Icon == model::PaletteIcon::Erase, "the eraser");
		});

		InSuite.Add("EditorPalette: the shipped palette covers every map character", [](TestContext& InContext)
		{
			const ConfigReader& Reader = GameContext::Get().GetManager<ConfigReader>();
			model::EditorPalette Palette;
			const bool bRead = Palette.FromConfig(Reader.GetConfig("configs/editor/palette.json"),
				[&Reader](const std::string& InId) { return Reader.HasConfig(InId) ? &Reader.GetConfig(InId) : nullptr; });
			InContext.Expect(bRead, "configs/editor/palette.json is read");
			InContext.Expect(Palette.GetErrors().empty(), "no bad entries");
			InContext.Expect(Palette.GetTabs().size() == 4, "walls, units, bonuses, tools");
			for ( const char Tile : { tiles::Brick, tiles::Stone, tiles::Wood, tiles::PlayerSpawn, tiles::EnemySpawn, tiles::Ammo, tiles::Repair, tiles::Empty } )
			{
				InContext.Expect(Palette.FindByTile(Tile) != nullptr, std::string("the palette paints '") + Tile + "'");
			}
		});
	}

	/** An audio port that only records what it is asked to do. */
	class FakeAudioOutput final : public ports::IAudioOutput
	{
	public:
		void Play(const std::string& InPath, const float InVolume) override { Played.emplace_back(InPath, InVolume); }
		void StopAll() override { ++StopCount; }
		void Preload(const std::string& InPath, const std::function<void(bool)>& InDone) override
		{
			Preloaded.push_back(InPath);
			if ( InDone )
			{
				InDone(true);
			}
		}
		void Unload(const std::string& InPath) override { Unloaded.push_back(InPath); }

		std::vector<std::pair<std::string, float>> Played;
		std::vector<std::string> Preloaded;
		std::vector<std::string> Unloaded;
		int StopCount = 0;
	};

	/** A storage port that keeps its files in memory. */
	class FakeStorage final : public ports::IStorage
	{
	public:
		bool Exists(const std::string& InName) const override { return Files.count(InName) > 0; }
		std::optional<std::string> ReadText(const std::string& InName) const override
		{
			const auto FileIt = Files.find(InName);
			return FileIt != Files.end() ? std::optional<std::string>(FileIt->second) : std::nullopt;
		}
		bool WriteText(const std::string& InName, const std::string& InText) override { Files[InName] = InText; return true; }

		std::map<std::string, std::string> Files;
	};

	void AddPortTests(TestSuite& InSuite)
	{
		InSuite.Add("JsonValue: ToString writes text that parses back to the same value", [](TestContext& InContext)
		{
			model::JsonValue::Object Members;
			Members.emplace("name", model::JsonValue("a \"quoted\"\nline\t\\"));
			Members.emplace("count", model::JsonValue(-7));
			Members.emplace("ratio", model::JsonValue(0.25));
			Members.emplace("on", model::JsonValue(true));
			Members.emplace("empty", model::JsonValue(model::JsonValue::Array()));
			Members.emplace("list", model::JsonValue(model::JsonValue::Array{ model::JsonValue(1), model::JsonValue("two"), model::JsonValue() }));
			model::JsonValue::Object Inner;
			Inner.emplace("x", model::JsonValue(3));
			Members.emplace("inner", model::JsonValue(std::move(Inner)));
			const model::JsonValue Original{ std::move(Members) };

			bool bOk = false;
			const model::JsonValue Parsed = model::JsonValue::Parse(Original.ToString(), &bOk);
			InContext.Expect(bOk, "the text is valid JSON");
			InContext.Expect(Parsed == Original, "the value survives the round trip");
		});

		InSuite.Add("ProfileSerializer: a profile survives a round trip and a damaged file gives defaults", [](TestContext& InContext)
		{
			PlayerProfile Original;
			Original.Coins = 123;
			Original.BestScore = 4567;
			Original.LevelsCompleted = 3;
			Original.Upgrades["armor"] = 2;
			Original.Upgrades["speed"] = 1;
			Original.CurrentLanguage = Language::Ukrainian;
			Original.bSoundEnabled = false;
			Original.FpsLimit = 144;
			Original.bVerticalSync = false;
			Original.CurrentWindowMode = WindowMode::Borderless;
			Original.WindowScale = 75;
			Original.WindowPosition = PixelPoint{ -20, 30 };
			Original.bHdTextures = false;
			Original.bSmoothPixels = true;
			Original.EffectsVolume = 40;

			const auto Node = JsonConfigNode::Parse(ProfileSerializer::ToJson(Original), "test");
			const PlayerProfile Read = ProfileSerializer::FromJson(*Node);
			InContext.Expect(Read.Coins == 123 && Read.BestScore == 4567 && Read.LevelsCompleted == 3, "progress");
			InContext.Expect(Read.Upgrades == Original.Upgrades, "upgrades");
			InContext.Expect(Read.CurrentLanguage == Language::Ukrainian && !Read.bSoundEnabled && Read.FpsLimit == 144 && !Read.bVerticalSync, "settings");
			InContext.Expect(Read.CurrentWindowMode == WindowMode::Borderless && Read.WindowScale == 75, "window");
			InContext.Expect(Read.WindowPosition && Read.WindowPosition->X == -20 && Read.WindowPosition->Y == 30, "window position");
			InContext.Expect(!Read.bHdTextures && Read.bSmoothPixels && Read.EffectsVolume == 40, "textures and volume");

			const auto Empty = JsonConfigNode::Parse("{}", "test");
			const PlayerProfile Fresh = ProfileSerializer::FromJson(*Empty);
			InContext.Expect(Fresh.Coins == 0 && Fresh.bSoundEnabled && Fresh.FpsLimit == 60 && !Fresh.WindowPosition, "missing keys give defaults");
		});

		InSuite.Add("ScreenLayout: any aspect ratio fills the window, the design area stays whole and centred", [](TestContext& InContext)
		{
			const model::Vec2f Design(1024.0f, 864.0f);
			const std::vector<std::pair<std::string, model::Vec2f>> Frames = {
				{ "4:3", { 1024.0f, 768.0f } }, { "16:9", { 1920.0f, 1080.0f } }, { "16:10", { 1680.0f, 1050.0f } },
				{ "21:9", { 2560.0f, 1080.0f } }, { "5:4", { 1280.0f, 1024.0f } }, { "phone 9:19.5", { 1080.0f, 2340.0f } },
				{ "tablet 3:4", { 1536.0f, 2048.0f } }, { "the design size", Design } };
			for ( const auto& [Name, Frame] : Frames )
			{
				const model::ScreenLayout Layout(Design, Frame);
				const model::Vec2f Visible = Layout.GetVisibleSize();
				InContext.ExpectNear(Visible.X * Layout.GetScale(), Frame.X, 0.5f, Name + ": the visible width fills the window");
				InContext.ExpectNear(Visible.Y * Layout.GetScale(), Frame.Y, 0.5f, Name + ": the visible height fills the window");
				InContext.Expect(Visible.X >= Design.X - 0.01f && Visible.Y >= Design.Y - 0.01f, Name + ": the design area is never cut");
				InContext.Expect(std::fabs(Visible.X - Design.X) < 0.01f || std::fabs(Visible.Y - Design.Y) < 0.01f, Name + ": the design area touches two edges");
				const model::RectF Rect = Layout.GetVisibleRect();
				InContext.ExpectNear(Rect.GetCenter().X, Design.X / 2, 0.01f, Name + ": centred across");
				InContext.ExpectNear(Rect.GetCenter().Y, Design.Y / 2, 0.01f, Name + ": centred up and down");
			}

			const model::ScreenLayout Wide(Design, model::Vec2f(1920.0f, 1080.0f));
			InContext.ExpectNear(Wide.GetScale(), 1.25f, 0.001f, "16:9 scale");
			InContext.ExpectNear(Wide.GetDesignOffset().X, 256.0f, 0.01f, "16:9: 256 extra units on each side");
			const model::Vec2f TopRight = Wide.GetAnchor(model::ScreenAnchor::TopRight);
			const model::Vec2f BottomLeft = Wide.GetAnchor(model::ScreenAnchor::BottomLeft);
			InContext.Expect(std::fabs(TopRight.X - 1280.0f) < 0.01f && std::fabs(TopRight.Y - 864.0f) < 0.01f, "16:9: the top right corner");
			InContext.Expect(std::fabs(BottomLeft.X + 256.0f) < 0.01f && std::fabs(BottomLeft.Y) < 0.01f, "16:9: the bottom left corner is left of the design area");
			const model::Vec2f Placed = Wide.Place(model::ScreenAnchor::TopLeft, model::Vec2f(16.0f, -48.0f));
			InContext.Expect(std::fabs(Placed.X + 240.0f) < 0.01f && std::fabs(Placed.Y - 816.0f) < 0.01f, "16:9: a point placed from the top left corner");

			const model::ScreenLayout Old(Design, model::Vec2f(1024.0f, 768.0f));
			InContext.ExpectNear(Old.GetVisibleSize().X, 1152.0f, 0.01f, "4:3 is wider than the design area");
			const model::ScreenLayout Phone(Design, model::Vec2f(1080.0f, 1920.0f));
			InContext.Expect(Phone.GetDesignOffset().X == 0.0f && Phone.GetDesignOffset().Y > 400.0f, "a tall phone gets the extra room above and below");
			const model::Vec2f Bottom = Phone.GetAnchor(model::ScreenAnchor::Bottom);
			InContext.Expect(std::fabs(Bottom.X - 512.0f) < 0.01f && Bottom.Y < 0.0f, "a tall phone: the bottom edge is below the design area");

			const model::ScreenLayout Broken(Design, model::Vec2f(0.0f, 0.0f));
			InContext.Expect(Broken.GetScale() == 1.0f && Broken.GetVisibleSize().X == Design.X, "an empty window falls back to the design size");
		});

		InSuite.Add("Splash: a small window centred on the work area, never bigger than it", [](TestContext& InContext)
		{
			const PixelRect Work = { 0, 0, 1920, 1040 };
			const PixelRect Rect = ComputeSplashRect({ 512, 432 }, Work);
			InContext.Expect(Rect.Left == 704 && Rect.Top == 304 && Rect.Right == 1216 && Rect.Bottom == 736, "centred on a 1080p work area");
			const PixelRect Second = ComputeSplashRect({ 512, 432 }, { -1280, 100, 0, 1124 });
			InContext.Expect(Second.Left == -896 && Second.Top == 396, "a work area left of the primary one");
			const PixelRect Tiny = ComputeSplashRect({ 512, 432 }, { 0, 0, 400, 300 });
			InContext.Expect(Tiny.Left == 0 && Tiny.Top == 0 && Tiny.Right == 400 && Tiny.Bottom == 300, "a tiny area: the splash fills it");
			const PixelSize Size = GetSplashSize(GameContext::Get().GetManager<UiStyle>().GetSplash().Width);
			InContext.ExpectNear(static_cast<float>(Size.Width) / Size.Height, DesignWidth / DesignHeight, 0.01f, "the splash has the design proportions");
			InContext.Expect(Size.Width < DesignWidth, "and is smaller than the design size");
		});

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		InSuite.Add("Splash: a borderless splash shows while the game window waits hidden for its saved place", [](TestContext& InContext)
		{
			const HWND Window = CheckedCast<cocos2d::GLViewImpl>(cocos2d::Director::getInstance()->getOpenGLView())->getWin32Window();
			const PlayerProfile& Saved = GameContext::Get().GetManager<ProfileManager>().Get();
			const PixelPoint StartPosition = GetWindowPosition();
			bool bShownEvent = false;
			auto Listener = cocos2d::Director::getInstance()->getEventDispatcher()->addCustomEventListener(GameWindowShownEvent,
				[&bShownEvent](cocos2d::EventCustom*) { bShownEvent = true; });

			SplashWindow Splash;
			InContext.Expect(Splash.Open(GetSplashSize(480), false, viewmodel::SplashStyle()) && Splash.IsOpen(), "the splash opens");
			const PixelRect Rect = Splash.GetRect();
			InContext.Expect(Rect.Right - Rect.Left == GetSplashSize(480).Width && Rect.Bottom - Rect.Top == GetSplashSize(480).Height, "at the splash size, without a frame");
			HideGameWindowUntilPlaced(false);
			InContext.Expect(IsGameWindowHiddenUntilPlaced() && !IsWindowVisible(Window), "the game window is hidden");

			ApplyWindowMode(Saved.CurrentWindowMode, Saved.WindowScale, StartPosition);
			InContext.Expect(!IsGameWindowHiddenUntilPlaced() && IsWindowVisible(Window) && bShownEvent, "shown once placed, and it says so");
			const PixelSize Expected = GetExpectedWindowedSize(Saved.WindowScale);
			InContext.Expect(GetWindowClientSize().Width == Expected.Width && GetWindowClientSize().Height == Expected.Height, "at the saved size");
			InContext.Expect(GetWindowPosition().X == StartPosition.X && GetWindowPosition().Y == StartPosition.Y, "at the saved place");
			Splash.Close();
			InContext.Expect(!Splash.IsOpen(), "the splash closes");
			cocos2d::Director::getInstance()->getEventDispatcher()->removeEventListener(Listener);
		});
#endif

		InSuite.Add("Showcase: --window=WxH sets the window size of the pictures", [](TestContext& InContext)
		{
			const auto Wide = ParseShowcaseWindow("TanksClone.exe --showcase out --window=1600x900 --background");
			InContext.Expect(Wide && Wide->Width == 1600 && Wide->Height == 900, "a size in the middle of the line");
			const auto Last = ParseShowcaseWindow("--showcase out --window=1024X768");
			InContext.Expect(Last && Last->Width == 1024 && Last->Height == 768, "a size at the end, capital X");
			InContext.Expect(!ParseShowcaseWindow("--showcase out") && !ParseShowcaseWindow("--window=big") && !ParseShowcaseWindow("--window=0x5"), "none or a bad one");
		});

		InSuite.Add("TextScales: sizes cycle, unknown values are Small, config factors override the defaults", [](TestContext& InContext)
		{
			using model::TextScales;
			using model::TextSize;
			InContext.Expect(TextScales::Next(TextSize::Small) == TextSize::Medium && TextScales::Next(TextSize::Medium) == TextSize::Large
				&& TextScales::Next(TextSize::Large) == TextSize::Small, "Small -> Medium -> Large -> Small");
			InContext.Expect(TextScales::FromIndex(2) == TextSize::Large && TextScales::FromIndex(7) == TextSize::Small && TextScales::FromIndex(-1) == TextSize::Small, "stored numbers");

			const TextScales Defaults;
			InContext.Expect(Defaults.Get(TextSize::Small) < 1.0f && Defaults.Get(TextSize::Medium) == 1.0f && Defaults.Get(TextSize::Large) > 1.0f,
				"by default Small is a little below the design size");

			const auto Node = JsonConfigNode::Parse("{\"text_scales\": [0.8, 1.1, 0.1]}", "test");
			const TextScales Read = TextScales::FromConfig(*Node);
			InContext.ExpectNear(Read.Get(TextSize::Small), 0.8f, 0.001f, "small from the config");
			InContext.ExpectNear(Read.Get(TextSize::Medium), 1.1f, 0.001f, "medium from the config");
			InContext.ExpectNear(Read.Get(TextSize::Large), TextScales::MinScale, 0.001f, "a tiny factor is raised to the minimum");
			const auto Short = JsonConfigNode::Parse("{\"text_scales\": [0.95]}", "test");
			InContext.ExpectNear(TextScales::FromConfig(*Short).Get(TextSize::Large), Defaults.Get(TextSize::Large), 0.001f, "missing factors keep their defaults");

			const TextScales& Shipped = GameContext::Get().GetManager<UiStyle>().GetWidgets().TextScales;
			InContext.Expect(Shipped.Get(TextSize::Small) < Shipped.Get(TextSize::Medium) && Shipped.Get(TextSize::Medium) < Shipped.Get(TextSize::Large),
				"the shipped sizes grow from Small to Large");

			PlayerProfile Original;
			InContext.Expect(Original.CurrentTextSize == TextSize::Small, "a new profile uses Small");
			Original.CurrentTextSize = TextSize::Large;
			const auto Saved = JsonConfigNode::Parse(ProfileSerializer::ToJson(Original), "test");
			InContext.Expect(ProfileSerializer::FromJson(*Saved).CurrentTextSize == TextSize::Large, "the text size survives saving the profile");
		});

		InSuite.Add("Storage: an old profile.plist is converted once to profile.json", [](TestContext& InContext)
		{
			FakeStorage Storage;
			InContext.Expect(!view::cocos::MigrateLegacyProfile(Storage), "nothing to convert in an empty storage");
			Storage.Files["profile.plist"] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?><!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"
				"<plist version=\"1.0\"><dict><key>coins</key><integer>42</integer><key>sound</key><false/>"
				"<key>upgrades</key><dict><key>armor</key><integer>2</integer></dict></dict></plist>";
			InContext.Expect(view::cocos::MigrateLegacyProfile(Storage), "the old file is converted");
			InContext.Expect(Storage.Exists("profile.plist"), "the old file stays");

			const auto Node = JsonConfigNode::Parse(Storage.Files["profile.json"], "test");
			const PlayerProfile Read = ProfileSerializer::FromJson(*Node);
			InContext.Expect(Read.Coins == 42 && !Read.bSoundEnabled && Read.Upgrades.count("armor") == 1 && Read.Upgrades.at("armor") == 2, "the progress carries over");
			InContext.Expect(!view::cocos::MigrateLegacyProfile(Storage), "an existing profile.json is never overwritten");
		});

		InSuite.Add("AudioManager: sounds go through the audio port, nothing plays while disabled", [](TestContext& InContext)
		{
			AudioManager& Audio = GameContext::Get().GetManager<AudioManager>();
			const bool bWasEnabled = Audio.IsEnabled();
			const float WasVolume = Audio.GetVolume();
			FakeAudioOutput Fake;
			Audio.SetOutput(&Fake);
			Audio.SetEnabled(true);
			Audio.SetVolume(0.5f);

			Audio.Play(ids::sounds::Pause);
			InContext.Expect(Fake.Played.size() == 1 && Fake.Played[0].first == "sounds/pause.wav" && Fake.Played[0].second == 0.5f, "the file of the id and the volume reach the port");
			Audio.Play(model::ResourceId());
			Audio.Play(model::ResourceId("sfx.no_such_sound"));
			InContext.ExpectEqual(Fake.Played.size(), std::size_t(1), "an empty or unknown id plays nothing");

			Audio.SetEnabled(false);
			InContext.ExpectEqual(Fake.StopCount, 1, "disabling stops the playing sounds");
			Audio.Play(ids::sounds::Pause);
			InContext.ExpectEqual(Fake.Played.size(), std::size_t(1), "nothing plays while disabled");

			Audio.SetEnabled(bWasEnabled);
			Audio.SetVolume(WasVolume);
			Audio.SetOutput(nullptr);
		});

		InSuite.Add("PlayerControls: actions become commands and the newest held movement wins", [](TestContext& InContext)
		{
			using model::InputAction;
			std::vector<CommandType> Commands;
			PlayerControls Controls([&Commands](const CommandType InCommand) { Commands.push_back(InCommand); });
			const auto Press = [&Controls](const InputAction InAction) { Controls.OnAction(model::InputEvent{ InAction, true }); };
			const auto Release = [&Controls](const InputAction InAction) { Controls.OnAction(model::InputEvent{ InAction, false }); };

			Press(InputAction::MoveUp);
			Press(InputAction::MoveRight);
			Release(InputAction::MoveRight);
			Release(InputAction::MoveUp);
			InContext.Expect(Commands == std::vector<CommandType>({ CommandType::MoveForward, CommandType::MoveRight, CommandType::MoveForward, CommandType::Stop }),
				"releasing the newest movement goes back to the one still held, then stops");

			Commands.clear();
			Press(InputAction::Fire);
			Press(InputAction::Ability1);
			Press(InputAction::SwitchWeapon);
			Press(InputAction::UiConfirm);
			Release(InputAction::Fire);
			InContext.Expect(Commands == std::vector<CommandType>({ CommandType::StartFiring, CommandType::UseAbility, CommandType::SwitchWeapon, CommandType::StopFiring }),
				"gameplay actions; menu actions are ignored");
		});

		InSuite.Add("PlayerControls: fire is held until released; ReleaseAll lets go of everything", [](TestContext& InContext)
		{
			using model::InputAction;
			std::vector<CommandType> Commands;
			PlayerControls Controls([&Commands](const CommandType InCommand) { Commands.push_back(InCommand); });
			const auto Press = [&Controls](const InputAction InAction) { Controls.OnAction(model::InputEvent{ InAction, true }); };

			Press(InputAction::Fire);
			Press(InputAction::Fire);
			InContext.Expect(Controls.IsFireHeld() && Commands == std::vector<CommandType>({ CommandType::StartFiring }), "a repeated press does not start firing twice");

			Commands.clear();
			Press(InputAction::MoveUp);
			Controls.ReleaseAll();
			InContext.Expect(!Controls.IsFireHeld() && !Controls.IsMoveHeld(), "nothing is held after ReleaseAll");
			InContext.Expect(Commands == std::vector<CommandType>({ CommandType::MoveForward, CommandType::StopFiring, CommandType::Stop }), "the tank stops firing and moving");

			Commands.clear();
			Controls.ReleaseAll();
			Controls.OnAction(model::InputEvent{ InputAction::Fire, false });
			InContext.Expect(Commands.empty(), "nothing to let go of twice; a release without a press sends nothing");
		});

		InSuite.Add("Input: the Cocos adapter turns engine key codes into keyboard buttons", [](TestContext& InContext)
		{
			using Code = cocos2d::EventKeyboard::KeyCode;
			using view::cocos::CocosKeyboardInput;
			InContext.Expect(view::cocos::CocosKeyboardInput::ToKey(Code::KEY_W) == model::Key::W && view::cocos::CocosKeyboardInput::ToKey(Code::KEY_CAPITAL_W) == model::Key::W, "both letter cases");
			InContext.Expect(view::cocos::CocosKeyboardInput::ToKey(Code::KEY_3) == model::Key::Digit3 && view::cocos::CocosKeyboardInput::ToKey(Code::KEY_F3) == model::Key::F3, "digits and F-keys");
			InContext.Expect(view::cocos::CocosKeyboardInput::ToKey(Code::KEY_BACK) == model::Key::Escape, "the phone's Back is Esc");
			InContext.Expect(view::cocos::CocosKeyboardInput::ToKey(Code::KEY_KP_ENTER) == model::Key::KeypadEnter, "keypad Enter");
		});
	}

	// The example of docs/DATA_DRIVEN_PLAN.md, section 3.2, plus the error cases.
	constexpr const char* TestInputConfig = R"({
		"contexts": {
			"menu": { "ui_up": [ { "key": "w" } ], "ui_back": [ { "key": "escape" } ], "ui_confirm": [ { "key": "space" } ] },
			"gameplay": {
				"move_up": [ { "key": "up" }, { "key": "W" } ],
				"fire": [ { "key": "space" }, { "pad": "a" } ],
				"ability_1": [ { "key": "e" } ],
				"switch_weapon": [ { "key": "e" }, { "key": "q" } ],
				"pause": [ { "key": "escape" }, { "pad": "start", "player": 2 } ],
				"jump": [ { "key": "j" } ],
				"move_down": [ { "key": "nonsense" }, { "key": "s" } ]
			}
		},
		"debug": { "toggle_stats": [ { "key": "f3" } ] }
	})";

	model::InputButton KeyButton(const model::Key InKey)
	{
		return model::InputButton::FromKey(InKey);
	}

	void AddInputActionTests(TestSuite& InSuite)
	{
		using model::InputAction;
		using model::InputContext;
		using model::Key;

		InSuite.Add("Input: bindings load from config; unknown names are reported and skipped", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(TestInputConfig, "test");
			const model::InputBindings Bindings = model::InputBindings::FromConfig(*Root, true);
			InContext.ExpectEqual(Bindings.GetButtons(InputContext::Gameplay, InputAction::MoveUp).size(), std::size_t(2), "two buttons on move_up");
			InContext.ExpectEqual(Bindings.GetButtons(InputContext::Gameplay, InputAction::MoveDown).size(), std::size_t(1), "the known button of move_down stays");
			InContext.ExpectEqual(Bindings.GetErrors().size(), std::size_t(2), "unknown action and unknown key are reported");
			InContext.Expect(Bindings.GetButtons(InputContext::Gameplay, InputAction::MoveUp)[0] == KeyButton(Key::Up), "config order is kept");
			InContext.Expect(Bindings.HasContext(InputContext::Debug), "debug section read with debug tools");
			InContext.Expect(!model::InputBindings::FromConfig(*Root, false).HasContext(InputContext::Debug), "and ignored without them");
		});

		InSuite.Add("Input: Enter, keypad Enter and Space confirm in menus (shipped config)", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(cocos2d::FileUtils::getInstance()->getStringFromFile("configs/input.json"), "input.json");
			const model::InputBindings Bindings = model::InputBindings::FromConfig(*Root, false);
			model::InputContextStack Stack(Bindings, InputContext::Menu);
			std::vector<model::InputEvent> Events;
			Stack.SetSink([&Events](const model::InputEvent& InEvent) { Events.push_back(InEvent); });
			for ( const auto Code : { cocos2d::EventKeyboard::KeyCode::KEY_ENTER, cocos2d::EventKeyboard::KeyCode::KEY_KP_ENTER, cocos2d::EventKeyboard::KeyCode::KEY_SPACE } )
			{
				Events.clear();
				Stack.OnButton(KeyButton(view::cocos::CocosKeyboardInput::ToKey(Code)), true);
				Stack.OnButton(KeyButton(view::cocos::CocosKeyboardInput::ToKey(Code)), false);
				InContext.Expect(!Events.empty() && Events[0].Action == InputAction::UiConfirm, "confirm for key code " + std::to_string(static_cast<int>(Code)));
			}
			Events.clear();
			Stack.OnButton(KeyButton(view::cocos::CocosKeyboardInput::ToKey(cocos2d::EventKeyboard::KeyCode::KEY_LEFT_ARROW)), true);
			InContext.Expect(!Events.empty() && Events[0].Action == InputAction::UiLeft, "Left adjusts a setting down");
			Events.clear();
			Stack.OnButton(KeyButton(view::cocos::CocosKeyboardInput::ToKey(cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_ARROW)), true);
			InContext.Expect(!Events.empty() && Events[0].Action == InputAction::UiRight, "Right adjusts a setting up");
		});

		InSuite.Add("Input: a release lost during a fade does not swallow the next press", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(cocos2d::FileUtils::getInstance()->getStringFromFile("configs/input.json"), "input.json");
			const model::InputBindings Bindings = model::InputBindings::FromConfig(*Root, false);
			model::InputContextStack Stack(Bindings, InputContext::Menu);
			int Confirms = 0;
			Stack.SetSink([&Confirms](const model::InputEvent& InEvent) { Confirms += InEvent.Action == InputAction::UiConfirm && InEvent.bPressed ? 1 : 0; });
			const model::InputButton Enter = KeyButton(model::Key::Enter);
			Stack.OnButton(Enter, true);
			Stack.ReleaseAll();
			Stack.OnButton(Enter, true);
			InContext.ExpectEqual(Confirms, 2, "Enter confirms again after the fade");
			Stack.ReleaseAll();
			Stack.OnButton(KeyButton(model::Key::Space), true);
			InContext.ExpectEqual(Confirms, 3, "and so does Space");
		});

		InSuite.Add("Input: only the active context gets buttons; a button may raise several actions", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(TestInputConfig, "test");
			const model::InputBindings Bindings = model::InputBindings::FromConfig(*Root, false);
			model::InputContextStack Stack(Bindings, InputContext::Gameplay);
			std::vector<model::InputEvent> Events;
			Stack.SetSink([&Events](const model::InputEvent& InEvent) { Events.push_back(InEvent); });

			Stack.OnButton(KeyButton(Key::Z), true);
			Stack.OnButton(KeyButton(Key::F3), true);
			InContext.Expect(Events.empty(), "unbound buttons (and the debug key without debug tools) are ignored");

			Stack.OnButton(KeyButton(Key::E), true);
			InContext.ExpectEqual(Events.size(), std::size_t(2), "e raises ability_1 and switch_weapon");
			Stack.OnButton(KeyButton(Key::E), false);
			InContext.Expect(Events.size() == 4 && !Events[3].bPressed, "and releases both");

			Events.clear();
			Stack.OnButton(model::InputButton::FromPad(model::PadButton::Start, 1), true);
			InContext.Expect(Events.empty(), "player 1's start is not player 2's binding");
			Stack.OnButton(model::InputButton::FromPad(model::PadButton::Start, 2), true);
			InContext.Expect(Events.size() == 1 && Events[0].Action == InputAction::Pause, "player 2's start pauses");
		});

		InSuite.Add("Input: two buttons on one action; hold to fire", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(TestInputConfig, "test");
			const model::InputBindings Bindings = model::InputBindings::FromConfig(*Root, false);
			model::InputContextStack Stack(Bindings, InputContext::Gameplay);
			std::vector<model::InputEvent> Events;
			Stack.SetSink([&Events](const model::InputEvent& InEvent) { Events.push_back(InEvent); });

			Stack.OnButton(KeyButton(Key::Up), true);
			Stack.OnButton(KeyButton(Key::W), true);
			Stack.OnButton(KeyButton(Key::Up), false);
			InContext.Expect(Stack.IsHeld(InputAction::MoveUp) && Events.size() == 1, "still held while w is down; pressed once");
			Stack.OnButton(KeyButton(Key::W), false);
			InContext.Expect(!Stack.IsHeld(InputAction::MoveUp) && Events.size() == 2 && !Events[1].bPressed, "released with the last button");

			Stack.OnButton(KeyButton(Key::Space), true);
			Stack.OnButton(KeyButton(Key::Space), true);
			InContext.Expect(Stack.IsHeld(InputAction::Fire) && Events.size() == 3, "fire held; a repeat is not a new press");
		});

		InSuite.Add("Input: pushing the menu releases held gameplay actions and blocks gameplay until popped", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(TestInputConfig, "test");
			const model::InputBindings Bindings = model::InputBindings::FromConfig(*Root, false);
			model::InputContextStack Stack(Bindings, InputContext::Gameplay);
			std::vector<model::InputEvent> Events;
			Stack.SetSink([&Events](const model::InputEvent& InEvent) { Events.push_back(InEvent); });

			Stack.OnButton(KeyButton(Key::Space), true);
			const auto Menu = Stack.Push(InputContext::Menu);
			InContext.Expect(Stack.GetActive() == InputContext::Menu, "menu on top");
			InContext.Expect(!Stack.IsHeld(InputAction::Fire) && Events.size() == 2 && !Events[1].bPressed, "fire released when the menu opens");

			Events.clear();
			Stack.OnButton(KeyButton(Key::Space), false);
			Stack.OnButton(KeyButton(Key::Space), true);
			InContext.Expect(Events.size() == 1 && Events[0].Action == InputAction::UiConfirm, "space confirms in the menu, never fires");
			Stack.OnButton(KeyButton(Key::Space), false);

			Events.clear();
			Stack.OnButton(KeyButton(Key::Escape), true);
			InContext.Expect(!Events.empty() && Events[0].Action == InputAction::UiBack, "Esc is back in the menu");
			Stack.Remove(Menu);
			Stack.OnButton(KeyButton(Key::Escape), false);
			Stack.OnButton(KeyButton(Key::Escape), true);
			InContext.Expect(Stack.GetActive() == InputContext::Gameplay && Events.back().Action == InputAction::Pause && Events.back().bPressed,
				"after the pop Esc pauses again; nothing stays stuck");
		});

		InSuite.Add("Input: contexts are removed by id in any order; key names round-trip", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(TestInputConfig, "test");
			const model::InputBindings Bindings = model::InputBindings::FromConfig(*Root, false);
			model::InputContextStack Stack(Bindings, InputContext::Menu);
			const auto Game = Stack.Push(InputContext::Gameplay);
			const auto Editor = Stack.Push(InputContext::Editor);
			Stack.Remove(Game);
			InContext.Expect(Stack.GetActive() == InputContext::Editor, "the scene that left first does not pop the newer one");
			Stack.Remove(Editor);
			InContext.Expect(Stack.GetActive() == InputContext::Menu, "back to the base");

			InContext.Expect(model::FindKey("keypad_enter") == Key::KeypadEnter && model::FindKey("F12") == Key::F12, "names, any case");
			InContext.Expect(model::GetButtonLabel(KeyButton(Key::Space)) == "SPACE", "HUD label");
		});
	}

	void AddUiStyleTests(TestSuite& InSuite)
	{
		InSuite.Add("UI style: fields keep their defaults and missing keys are recorded with their path", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(R"({"top": {"padding": 5, "anchor": "top_right"}, "slots": {"max": "many"}})", "test");
			std::vector<std::string> Missing;
			const viewmodel::HudLayout Hud = viewmodel::HudLayout::FromConfig(Root.get(), Missing);
			InContext.ExpectNear(Hud.TopPadding, 5.0f, 0.001f, "a present value is read");
			InContext.Expect(Hud.TopAnchor == model::ScreenAnchor::TopRight, "anchors are read by name");
			InContext.ExpectNear(Hud.TopFontSize, viewmodel::HudLayout().TopFontSize, 0.001f, "a missing value keeps its default");
			InContext.Expect(Hud.MaxSlots == viewmodel::HudLayout().MaxSlots, "a value of the wrong type keeps its default");
			const auto Has = [&Missing](const std::string& InPath) { return std::find(Missing.begin(), Missing.end(), InPath) != Missing.end(); };
			InContext.Expect(Has("top.font_size") && Has("slots.max") && Has("banner.hold"), "missing and wrong keys are recorded with their path");
			InContext.Expect(!Has("top.padding") && !Has("top.anchor"), "read keys are not");

			std::vector<std::string> NoFile;
			const viewmodel::MenuLayout Menu = viewmodel::MenuLayout::FromConfig(nullptr, NoFile);
			InContext.Expect(Menu.Settings.VolumeStep == viewmodel::SettingsLayout().VolumeStep && !NoFile.empty(), "a missing file gives the defaults and reports every key");
		});

		InSuite.Add("UI style: the shipped configs/ui files have every key", [](TestContext& InContext)
		{
			const ConfigReader& Reader = GameContext::Get().GetManager<ConfigReader>();
			const auto Check = [&InContext, &Reader](const std::string& InId, const auto& InLoad)
			{
				std::vector<std::string> Missing;
				InLoad(Reader.HasConfig(InId) ? &Reader.GetConfig(InId) : nullptr, Missing);
				InContext.Expect(Missing.empty(), InId + (Missing.empty() ? "" : " misses " + Missing.front()));
			};
			Check("configs/ui/widgets.json", &viewmodel::WidgetStyle::FromConfig);
			Check("configs/ui/menu.json", &viewmodel::MenuLayout::FromConfig);
			Check("configs/ui/hud.json", &viewmodel::HudLayout::FromConfig);
			Check("configs/ui/editor.json", &viewmodel::EditorStyle::FromConfig);
			Check("configs/ui/splash.json", &viewmodel::SplashStyle::FromConfig);
		});

		InSuite.Add("UI style: the editor layout follows its settings", [](TestContext& InContext)
		{
			model::EditorLayoutSettings Wide;
			Wide.Gap = 20.0f;
			const model::RectF Bounds(0.0f, 0.0f, 600.0f, 200.0f);
			const model::EditorLayoutResult Default = model::EditorLayout(4, 2).Compute(Bounds, 6);
			const model::EditorLayoutResult Spaced = model::EditorLayout(4, 2, Wide).Compute(Bounds, 6);
			InContext.Expect(Spaced.Tabs[1].GetMinX() - Spaced.Tabs[0].GetMaxX() > Default.Tabs[1].GetMinX() - Default.Tabs[0].GetMaxX(), "a bigger gap spreads the tabs");
		});
	}

	void AddResourceIdTests(TestSuite& InSuite)
	{
		InSuite.Add("ResourceId: views a name and compares by it", [](TestContext& InContext)
		{
			constexpr model::ResourceId A{ "sfx.a" };
			InContext.Expect(A.IsValid() && !model::ResourceId().IsValid(), "an empty id is invalid");
			InContext.Expect(A == model::ResourceId("sfx.a") && A != ids::sounds::Pause, "equal by name");
			InContext.Expect(A.GetName() == "sfx.a", "the name");
		});

		InSuite.Add("Handle: the last handle frees the object, copies and moves keep it", [](TestContext& InContext)
		{
			int Destroyed = 0;
			class Counted : public model::RefCounted
			{
			public:
				explicit Counted(int* const InDestroyed) : Destroyed(InDestroyed) {}
			private:
				~Counted() override { ++*Destroyed; }
				int* Destroyed;
			};

			model::Handle<Counted> First(new Counted(&Destroyed));
			InContext.ExpectEqual(First->GetRefCount(), 1, "one handle");
			{
				model::Handle<Counted> Copy = First;
				InContext.ExpectEqual(First->GetRefCount(), 2, "a copy counts");
				model::Handle<Counted> Moved = std::move(Copy);
				InContext.Expect(!Copy && Moved && First->GetRefCount() == 2, "a move does not count twice");
			}
			InContext.Expect(Destroyed == 0 && First->GetRefCount() == 1, "still alive with one handle");
			First.Reset();
			InContext.ExpectEqual(Destroyed, 1, "freed when the last handle is released");
		});

		InSuite.Add("ResourceLoadJob: loads a set step by step and holds it until taken", [](TestContext& InContext)
		{
			ResourceManager& Manager = GameContext::Get().GetManager<ResourceManager>();
			ResourceSet Set;
			Set.Atlases = { "atlas.tileset_desert", "atlas.no_such_atlas" };
			Set.Sounds = { "sfx.pause" };
			ResourceLoadJob Job(Set, false);
			InContext.Expect(!Job.IsFinished() && Job.GetProgress() == 0.0f, "nothing is loaded before the first step");

			float Previous = 0.0f;
			int Steps = 0;
			while ( !Job.IsFinished() )
			{
				Job.Step();
				InContext.Expect(Job.GetProgress() > Previous, "progress grows with every step");
				Previous = Job.GetProgress();
				++Steps;
			}
			InContext.ExpectEqual(Steps, 3, "one step per resource");
			InContext.ExpectEqual(Job.GetProgress(), 1.0f, "progress ends at 1");
			InContext.ExpectEqual(Job.GetFailedCount(), std::size_t(1), "the unknown atlas is counted as failed");
			InContext.Expect(Manager.IsAtlasLoaded("atlas.tileset_desert"), "the atlas is loaded while the job holds it");

			{
				ResourceHolder Taken = Job.TakeResources();
				InContext.Expect(Taken.GetAtlases().size() == 1 && Taken.GetSounds().size() == 1, "the holder has what loaded");
				InContext.Expect(Manager.IsSoundLoaded("sfx.pause"), "the sound is loaded");
			}
			InContext.Expect(!Manager.IsAtlasLoaded("atlas.tileset_desert"), "the atlas unloads with the holder");
			InContext.Expect(!Manager.IsSoundLoaded("sfx.pause"), "the sound unloads with the holder");

			ResourceLoadJob Empty(ResourceSet(), false);
			InContext.Expect(Empty.IsFinished() && Empty.GetProgress() == 1.0f, "an empty job is finished");
		});

		InSuite.Add("ResourceManifest: a level asks for its atlases and the sounds its tanks name", [](TestContext& InContext)
		{
			const ResourceManifest Manifest;
			LevelData First;
			InContext.Expect(LevelLoader::Load(LevelLoader::BuiltinId(0), First), "level 1 loads");
			const ResourceSet WithSounds = Manifest.GetLevelResources(First, true);
			InContext.ExpectEqual(WithSounds.Atlases.size(), size_t(2), "the tileset and one enemy");
			InContext.Expect(Contains(WithSounds.Sounds, "sfx.player_shot") && Contains(WithSounds.Sounds, "sfx.enemy_explode"), "player and enemy sounds");
			InContext.Expect(!Contains(WithSounds.Sounds, "sfx.boss_explode"), "no boss sound in level 1");
			InContext.Expect(Manifest.GetLevelResources(First, false).Sounds.empty(), "no sounds when the sound is off");
		});

		InSuite.Add("WorkQueue: work runs on another thread, results come back in order on Pump()", [](TestContext& InContext)
		{
			model::WorkQueue Queue;
			const std::thread::id MainThread = std::this_thread::get_id();
			std::vector<int> Results;
			bool bWorkedElsewhere = true;
			for ( int Index = 0; Index < 5; ++Index )
			{
				Queue.Submit<int>(
					[Index, MainThread, &bWorkedElsewhere]()
					{
						bWorkedElsewhere = bWorkedElsewhere && std::this_thread::get_id() != MainThread;
						return Index * 10;
					},
					[&Results, MainThread, &bWorkedElsewhere](const int InValue)
					{
						bWorkedElsewhere = bWorkedElsewhere && std::this_thread::get_id() == MainThread;
						Results.push_back(InValue);
					});
			}
			InContext.Expect(Results.empty(), "nothing is delivered before Pump");
			const auto Until = std::chrono::steady_clock::now() + std::chrono::seconds(5);
			while ( Results.size() < 5 && std::chrono::steady_clock::now() < Until )
			{
				Queue.Pump();
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			InContext.Expect(Results == std::vector<int>({ 0, 10, 20, 30, 40 }), "five results, in submit order");
			InContext.Expect(bWorkedElsewhere, "work off the main thread, done on it");
			InContext.ExpectEqual(Queue.GetPendingCount(), std::size_t(0), "nothing pending at the end");
		});

		InSuite.Add("WorkQueue: urgent jobs go first, Pump respects its budget", [](TestContext& InContext)
		{
			model::WorkQueue Queue;
			std::atomic<bool> bOpen{ false };
			std::vector<std::string> Order;
			std::atomic<bool> bStarted{ false };
			Queue.Submit<int>([&]() { bStarted = true; while ( !bOpen ) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); } return 0; }, [&Order](int) { Order.push_back("gate"); });
			while ( !bStarted ) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
			Queue.Submit<int>([]() { return 0; }, [&Order](int) { Order.push_back("low"); }, model::WorkPriority::Low);
			Queue.Submit<int>([]() { return 0; }, [&Order](int) { Order.push_back("normal"); });
			Queue.Submit<int>([]() { return 0; }, [&Order](int) { Order.push_back("high"); }, model::WorkPriority::High);
			bOpen = true;
			Queue.WaitIdle();

			InContext.ExpectEqual(Queue.Pump(std::chrono::microseconds(0)), std::size_t(1), "a zero budget still delivers one job");
			InContext.ExpectEqual(Queue.GetPendingCount(), std::size_t(3), "the rest wait for the next frame");
			Queue.Pump();
			InContext.Expect(Order == std::vector<std::string>({ "high", "gate", "normal", "low" }), "delivered high first, then normal in submit order, then low");
			InContext.ExpectEqual(Queue.GetPendingCount(), std::size_t(0), "all delivered");
		});

		InSuite.Add("WorkQueue: CancelAll drops waiting, running and finished jobs without calling them", [](TestContext& InContext)
		{
			model::WorkQueue Queue;
			std::atomic<bool> bOpen{ false };
			std::atomic<bool> bStarted{ false };
			int Delivered = 0;
			Queue.Submit<int>([&]() { bStarted = true; while ( !bOpen ) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); } return 0; }, [&Delivered](int) { ++Delivered; });
			Queue.Submit<int>([]() { return 0; }, [&Delivered](int) { ++Delivered; });
			while ( !bStarted ) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
			Queue.CancelAll();
			bOpen = true;
			Queue.WaitIdle();
			Queue.Pump();
			InContext.ExpectEqual(Delivered, 0, "nothing is delivered after CancelAll");
			InContext.ExpectEqual(Queue.GetPendingCount(), std::size_t(0), "nothing is pending");

			Queue.Submit<int>([]() { return 1; }, [&Delivered](int InValue) { Delivered += InValue; });
			Queue.WaitIdle();
			Queue.CancelAll();
			Queue.Pump();
			InContext.ExpectEqual(Delivered, 0, "a finished but undelivered job is dropped too");

			Queue.Submit<int>([]() { return 5; }, [&Delivered](int InValue) { Delivered += InValue; });
			Queue.WaitIdle();
			Queue.Pump();
			InContext.ExpectEqual(Delivered, 5, "the queue works as before after a cancel");
		});

		InSuite.Add("ResourceManager: a file is read on the loader thread", [](TestContext& InContext)
		{
			ResourceManager& Manager = GameContext::Get().GetManager<ResourceManager>();
			std::vector<unsigned char> Bytes;
			bool bDone = false;
			Manager.ReadFileAsync(PathOf(ids::configs::PlayerTank, model::ResourceKind::Config), [&](std::vector<unsigned char> InBytes) { Bytes = std::move(InBytes); bDone = true; });
			const auto Until = std::chrono::steady_clock::now() + std::chrono::seconds(5);
			while ( !bDone && std::chrono::steady_clock::now() < Until )
			{
				Manager.GetQueue().Pump();
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			InContext.Expect(bDone && Bytes.size() > 10 && Bytes[0] == '{', "the bytes of the config arrive");
		});

		InSuite.Add("Atlases: an async load decodes on the worker, shares one load and unloads with the last handle", [](TestContext& InContext)
		{
			ResourceManager& Manager = GameContext::Get().GetManager<ResourceManager>();
			const std::string Atlas = "atlas.tileset_desert";
			const std::string Frame = ResourceManifest::GetTilesetFrame("desert", "ground_1");
			InContext.Expect(!Manager.IsAtlasLoaded(Atlas), "not loaded before");

			AtlasHandle First;
			AtlasHandle Second;
			int Calls = 0;
			Manager.AcquireAtlasAsync(Atlas, [&](AtlasHandle InHandle) { First = std::move(InHandle); ++Calls; });
			Manager.AcquireAtlasAsync(Atlas, [&](AtlasHandle InHandle) { Second = std::move(InHandle); ++Calls; });
			InContext.Expect(!Manager.IsAtlasLoaded(Atlas), "still loading right after the request");
			const auto Until = std::chrono::steady_clock::now() + std::chrono::seconds(10);
			while ( Calls < 2 && std::chrono::steady_clock::now() < Until )
			{
				Manager.GetQueue().Pump();
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			InContext.ExpectEqual(Calls, 2, "both requests are answered");
			InContext.Expect(First && First.Get() == Second.Get(), "one atlas object for both");
			InContext.Expect(Manager.IsAtlasLoaded(Atlas) && HasFrame(Frame), "loaded with its frames");
			First.Reset();
			InContext.Expect(Manager.IsAtlasLoaded(Atlas), "kept by the other handle");
			Second.Reset();
			InContext.Expect(!Manager.IsAtlasLoaded(Atlas) && !HasFrame(Frame), "unloaded with the last handle");

			bool bFailed = false;
			bool bAnswered = false;
			Manager.AcquireAtlasAsync("atlas.no_such_atlas", [&](AtlasHandle InHandle) { bFailed = !InHandle; bAnswered = true; });
			InContext.Expect(bAnswered && bFailed, "an unknown atlas is answered at once with an empty handle");
		});

		InSuite.Add("Atlases: ids resolve to shipped files and none is unused", [](TestContext& InContext)
		{
			ResourceManager& Manager = GameContext::Get().GetManager<ResourceManager>();
			cocos2d::FileUtils* const Files = cocos2d::FileUtils::getInstance();
			const std::vector<std::string> Paths = Manager.GetTable().GetPaths(model::ResourceKind::Atlas);
			InContext.Expect(!Paths.empty(), "atlas ids are listed");
			for ( const std::string& Path : Paths )
			{
				InContext.Expect(Files->isFileExist(Path), "atlas file exists: " + Path);
			}
			const std::filesystem::path Root = std::filesystem::path(Files->fullPathForFilename("textures/common.plist")).parent_path();
			for ( const auto& File : std::filesystem::recursive_directory_iterator(Root) )
			{
				if ( File.path().extension() == ".plist" )
				{
					const std::string Relative = "textures/" + std::filesystem::relative(File.path(), Root).generic_string();
					InContext.Expect(std::find(Paths.begin(), Paths.end(), Relative) != Paths.end(), "atlas file is in the table: " + Relative);
				}
			}
			InContext.Expect(!Manager.AcquireAtlas("atlas.no_such_atlas"), "an unknown atlas gives an empty handle");
		});

		InSuite.Add("ResourceTable: ids map to paths, overrides replace them", [](TestContext& InContext)
		{
			const auto Base = JsonConfigNode::Parse(R"({"sounds":{"sfx.a":"sounds/a.wav"},"atlases":{"atlas.x":"textures/x.plist"}})", "test");
			model::ResourceTable Table;
			InContext.Expect(Table.FromConfig(*Base), "a valid table is read");
			const model::ResourceEntry* const Found = Table.Find(model::ResourceId("sfx.a"));
			InContext.Expect(Found && Found->Path == "sounds/a.wav" && Found->Kind == model::ResourceKind::Sound, "a sound id");
			InContext.Expect(Table.Find("sfx.missing") == nullptr, "an unknown id is not found");
			InContext.Expect(Table.GetIds(model::ResourceKind::Atlas).size() == 1, "ids by kind");

			const auto Override = JsonConfigNode::Parse(R"({"sounds":{"sfx.a":"mod/a.ogg","sfx.b":"mod/b.ogg"},"atlases":{"sfx.a":"bad"}})", "test");
			InContext.ExpectEqual(Table.ApplyOverrides(*Override), std::size_t(2), "two entries change, the one of another kind is refused");
			InContext.Expect(Table.Find("sfx.a")->Path == "mod/a.ogg" && Table.Find("sfx.b") != nullptr, "override and new id");

			const auto Duplicate = JsonConfigNode::Parse(R"({"sounds":{"x":"a"},"fonts":{"x":"b"}})", "test");
			model::ResourceTable Bad;
			InContext.Expect(!Bad.FromConfig(*Duplicate), "an id in two sections is reported");
		});

		InSuite.Add("ResourceTable: a section can be the path of another config", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(R"({"sounds":"sounds.json","fonts":"missing.json","configs":{"cfg.a":"a.json"}})", "test");
			const auto SoundsFile = JsonConfigNode::Parse(R"({"sounds":{"sfx.a":"a.wav"}})", "test");
			const model::ResourceTable::ConfigResolver Resolver = [&SoundsFile](const std::string& InPath) -> const model::IConfigNode*
			{
				return InPath == "sounds.json" ? SoundsFile.get() : nullptr;
			};
			model::ResourceTable Table;
			InContext.Expect(!Table.FromConfig(*Root, Resolver), "a missing section file is reported");
			InContext.Expect(Table.Find("sfx.a") && Table.Find("sfx.a")->Path == "a.wav", "the section from another file");
			InContext.Expect(Table.Find("cfg.a") != nullptr, "an inline section next to it still reads");
		});

		InSuite.Add("ResourceManager: every id in code resolves, every file exists and none is unused", [](TestContext& InContext)
		{
			ResourceManager& Manager = GameContext::Get().GetManager<ResourceManager>();
			cocos2d::FileUtils* const Files = cocos2d::FileUtils::getInstance();
			const auto CheckGroup = [&](const auto& InIds, const model::ResourceKind InKind)
			{
				for ( const model::ResourceId& Id : InIds )
				{
					const std::string Path = Manager.ResolvePath(Id, InKind);
					InContext.Expect(!Path.empty() && Files->isFileExist(Path), "id resolves to an existing file: " + std::string(Id.GetName()));
				}
			};
			CheckGroup(ids::sounds::All, model::ResourceKind::Sound);
			CheckGroup(ids::fonts::All, model::ResourceKind::Font);
			CheckGroup(ids::configs::All, model::ResourceKind::Config);

			const auto CheckFolder = [&](const model::ResourceKind InKind, const std::string& InExtension)
			{
				const std::vector<std::string> Paths = Manager.GetTable().GetPaths(InKind);
				const std::string Sample = Files->fullPathForFilename(Paths.front());
				const std::string Folder = Paths.front().substr(0, Paths.front().find('/') + 1);
				for ( const auto& File : std::filesystem::directory_iterator(std::filesystem::path(Sample).parent_path()) )
				{
					if ( File.path().extension() == InExtension )
					{
						const std::string Relative = Folder + File.path().filename().string();
						InContext.Expect(std::find(Paths.begin(), Paths.end(), Relative) != Paths.end(), "file is in the table: " + Relative);
					}
				}
			};
			CheckFolder(model::ResourceKind::Sound, ".wav");
			CheckFolder(model::ResourceKind::Font, ".ttf");
		});

		InSuite.Add("Sounds: a config names any sound of the resource table", [](TestContext& InContext)
		{
			ResourceManager& Manager = GameContext::Get().GetManager<ResourceManager>();
			const auto Mod = JsonConfigNode::Parse(R"({"sounds":{"sfx.test_laser":"sounds/pause.wav"}})", "test");
			Manager.ApplyOverrides(*Mod);

			const auto Config = JsonConfigNode::Parse(R"({"sound_explode":"sfx.test_laser","sound_hit":"sfx.no_such_sound",
				"weapons":[{"id":"laser","sound":"sfx.test_laser"},{"id":"quiet"}],
				"abilities":[{"type":"Buff","buff":"shield","sound":"sfx.player_shot"}]})", "test");
			const TankDef Def = TankLoader::ReadTank(*Config);
			InContext.Expect(Def.ExplodeSound.IsValid() && Def.ExplodeSound.GetName() == "sfx.test_laser", "a new sound id is usable from a config");
			InContext.Expect(!Def.HitSound.IsValid() && !Def.ArmorSound.IsValid(), "an unknown or missing sound id means silence");
			InContext.Expect(Def.Weapons.size() == 2 && Def.Weapons[0].FireSound.IsValid() && !Def.Weapons[1].FireSound.IsValid(), "weapon sounds");
			InContext.Expect(Def.Abilities.size() == 1 && Def.Abilities[0].ActivateSound == ids::sounds::PlayerShot, "ability sound");
			InContext.Expect(PathOf(Def.ExplodeSound, model::ResourceKind::Sound) == "sounds/pause.wav", "the id resolves to its file");
		});

		InSuite.Add("Sounds: shipped tanks name valid sounds", [](TestContext& InContext)
		{
			const ConfigReader& Reader = GameContext::Get().GetManager<ConfigReader>();
			const TankDef Player = TankLoader::ReadTank(Reader.GetConfig(PlayerTankPath()));
			InContext.Expect(Player.ExplodeSound.IsValid() && Player.HitSound.IsValid() && Player.ArmorSound.IsValid(), "the player hit and explode sounds");
			InContext.Expect(!Player.Weapons.empty() && Player.Weapons[0].FireSound.IsValid(), "the player cannon shot");
			for ( const std::string& Id : Reader.GetConfigIds(EnemiesFolder) )
			{
				InContext.Expect(TankLoader::ReadTank(Reader.GetConfig(Id)).ExplodeSound.IsValid(), "explode sound: " + Id);
			}
		});

		InSuite.Add("ResourceManager: a missing id gives an empty path", [](TestContext& InContext)
		{
			ResourceManager Manager;
			InContext.Expect(Manager.ResolvePath(model::ResourceId("sfx.no_such_sound"), model::ResourceKind::Sound).empty(), "unknown id");
			InContext.Expect(Manager.ResolvePath(ids::sounds::PlayerShot, model::ResourceKind::Font).empty(), "wrong kind");
			InContext.Expect(Manager.GetStatus().IsMissing("fonts", "sfx.player_shot"), "recorded missing");
		});

		InSuite.Add("ResourceStatusBook: unchecked, ok and missing; a missing one is reported once and forgotten on demand", [](TestContext& InContext)
		{
			model::ResourceStatusBook Book;
			InContext.Expect(Book.Get("atlas", "a") == model::ResourceState::Unchecked, "unchecked at first");
			Book.MarkOk("atlas", "a");
			InContext.Expect(Book.Get("atlas", "a") == model::ResourceState::Ok && Book.Get("frame", "a") == model::ResourceState::Unchecked, "ok, per kind");
			InContext.Expect(Book.MarkMissing("frame", "b"), "first failure is reported");
			InContext.Expect(!Book.MarkMissing("frame", "b"), "the second is not");
			InContext.Expect(Book.Count(model::ResourceState::Missing) == 1 && Book.GetMissing() == std::vector<std::string>{ "frame b" }, "listed");
			Book.ForgetMissing();
			InContext.Expect(Book.Get("frame", "b") == model::ResourceState::Unchecked && Book.Get("atlas", "a") == model::ResourceState::Ok, "missing forgotten, ok kept");
			InContext.Expect(Book.MarkMissing("frame", "b"), "reported again after forgetting");
		});

		InSuite.Add("ResourceManager: asked before the configs are read, it does not stick with an empty table (startup order)", [](TestContext& InContext)
		{
			ResourceManager Manager;
			const ConfigReader Empty;
			Manager.SetConfigReader(&Empty);
			InContext.Expect(Manager.ResolvePath(ids::fonts::Plain, model::ResourceKind::Font).empty(), "no path before the configs");
			InContext.Expect(!Manager.IsTableLoaded(), "the table is not marked loaded");
			InContext.ExpectEqual(Manager.GetStatus().Count(model::ResourceState::Missing), std::size_t(0), "nothing marked missing for good");
			Manager.SetConfigReader(nullptr);
			InContext.Expect(!Manager.ResolvePath(ids::fonts::Plain, model::ResourceKind::Font).empty(), "the font resolves once the configs are read");
			InContext.Expect(Manager.IsTableLoaded(), "then the table is loaded");
		});

		InSuite.Add("ResourceManager: a missing frame gives the placeholder, recorded once; a missing atlas is not tried again", [](TestContext& InContext)
		{
			ResourceManager Manager;
			cocos2d::SpriteFrame* const Missing = Manager.FindFrame("no_such_frame");
			InContext.Expect(Missing && Manager.IsPlaceholder(Missing), "placeholder instead of nullptr");
			InContext.Expect(Manager.FindFrame("no_such_frame") == Missing, "the same placeholder");
			InContext.Expect(Manager.GetStatus().IsMissing(ResourceManager::FrameKind, "no_such_frame"), "recorded missing");
			InContext.Expect(!Manager.IsPlaceholder(Manager.FindFrame("bonus_ammo")), "a real frame is returned as is");
			InContext.Expect(Manager.GetStatus().Get(ResourceManager::FrameKind, "bonus_ammo") == model::ResourceState::Ok, "recorded ok");
			InContext.Expect(!static_cast<bool>(Manager.AcquireAtlas("atlas.no_such_atlas")), "unknown atlas");
			InContext.Expect(Manager.GetStatus().IsMissing("atlases", "atlas.no_such_atlas"), "recorded missing");
			InContext.Expect(static_cast<bool>(Manager.AcquireAtlas("atlas.enemy_boss")), "a known atlas loads");
			InContext.Expect(Manager.GetStatus().Get("atlases", "atlas.enemy_boss") == model::ResourceState::Ok, "recorded ok");
		});

		InSuite.Add("ResourceManifest: global atlases and tilesets come from the referenced file", [](TestContext& InContext)
		{
			const ResourceManifest Manifest;
			InContext.Expect(Manifest.GetGlobalAtlases().size() == 3, "three global atlases");
			InContext.Expect(!Manifest.GetTilesetAtlas("desert").empty(), "tilesets are read");
		});
	}

	std::vector<int> GLifetimeLog;

	template <int TId>
	class LoggedManager
	{
	public:
		LoggedManager()
		{
			GLifetimeLog.push_back(TId);
		}

		~LoggedManager()
		{
			GLifetimeLog.push_back(-TId);
		}
	};

	void AddGameContextTests(TestSuite& InSuite)
	{
		using TestManagers = TypeList<LoggedManager<1>, LoggedManager<2>, LoggedManager<3>>;
		static_assert(TypeIndexOf<LoggedManager<3>, TestManagers>::value == 2, "TypeIndexOf");
		static_assert(IsInTypeList<LoggedManager<2>, TestManagers>::value && !IsInTypeList<int, TestManagers>::value, "IsInTypeList");

		InSuite.Add("ManagerSet: creates in list order, destroys in reverse, once", [](TestContext& InContext)
		{
			GLifetimeLog.clear();
			{
				ManagerSet<TestManagers> Managers;
				InContext.Expect(GLifetimeLog.empty() && !Managers.IsCreated(), "nothing before Create()");
				Managers.Create();
				Managers.Create();
				InContext.Expect(GLifetimeLog == std::vector<int>{ 1, 2, 3 }, "created once, in list order");
				InContext.Expect(&Managers.Get<LoggedManager<2>>() == &Managers.Get<LoggedManager<2>>(), "the same instance");
				Managers.Destroy();
				InContext.Expect(GLifetimeLog == std::vector<int>{ 1, 2, 3, -3, -2, -1 }, "destroyed in reverse order");
				Managers.Create();
			}
			InContext.Expect(GLifetimeLog.size() == 12 && GLifetimeLog.back() == -1, "the destructor destroys what is left");
		});

		InSuite.Add("GameContext: the game runs with every manager created", [](TestContext& InContext)
		{
			const GameContext& Context = GameContext::Get();
			InContext.Expect(Context.IsInitialized(), "initialized");
			InContext.Expect(&Context.GetManager<ConfigReader>() == &GameContext::Get().GetManager<ConfigReader>(), "one ConfigReader");
		});
	}
}

TestSuite CreateUnitTests()
{
	TestSuite Suite;
	AddMathTests(Suite);
	AddStatsTests(Suite);
	AddBuffTests(Suite);
	AddDataTests(Suite);
	AddResourceTests(Suite);
	AddTankTests(Suite);
	AddWallTests(Suite);
	AddWeaponTests(Suite);
	AddMovementTests(Suite);
	AddTurnTests(Suite);
	AddUpgradeTreeTests(Suite);
	AddNewWeaponTests(Suite);
	AddAiTests(Suite);
	AddObjectCastTests(Suite);
	AddEditorTests(Suite);
	AddGameContextTests(Suite);
	AddInputActionTests(Suite);
	AddUiStyleTests(Suite);
	AddResourceIdTests(Suite);
	AddPortTests(Suite);
	return Suite;
}

} // namespace tests
} // namespace tanks
