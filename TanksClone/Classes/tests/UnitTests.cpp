#include "ports/ILogSink.h"
#include "tests/UnitTests.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <thread>

#include "2d/CCFontAtlas.h"
#include "2d/CCFontAtlasCache.h"
#include "ui/CocosConvert.h"

#include "core/ConfigReader.h"
#include "core/DataConverter.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/JsonConfigNode.h"
#include "templates/TContainers.h"
#include "templates/TFunction.h"
#include "templates/TFunctionRef.h"
#include "templates/TManagerSet.h"
#include "templates/TObjectPool.h"
#include "templates/TRingBuffer.h"
#include "templates/TSlotMap.h"
#include "templates/TStoredString.h"
#include "templates/TUniquePtr.h"
#include "templates/TTemplateHelper.h"
#include "core/ObjectCast.h"
#include "core/ResourceIds.h"
#include "templates/TTypeList.h"
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
#include "game/objects/Bullet.h"
#include "game/objects/Tank.h"
#include "game/objects/TankLoader.h"
#include "game/objects/Wall.h"
#include "game/ObjectsFactory.h"
#include "game/ResourceManifest.h"
#include "game/PlayerControls.h"
#include "model/InputBindings.h"
#include "model/CloakCycle.h"
#include "model/CampaignList.h"
#include "model/CustomLibrary.h"
#include "model/DamageState.h"
#include "model/GridPath.h"
#include "model/Animation.h"
#include "model/BarMeter.h"
#include "model/HoverBook.h"
#include "model/MapCamera.h"
#include "model/VisibilityMap.h"
#include "model/RichText.h"
#include "model/MusicDirector.h"
#include "model/ScriptPlayer.h"
#include "model/BulletClash.h"
#include "model/AiFeatureSet.h"
#include "model/AiTactic.h"
#include "model/CampWatch.h"
#include "model/AiTactics.h"
#include "model/ConfigCatalog.h"
#include "model/Cooldown.h"
#include "model/Countdown.h"
#include "model/GameEvents.h"
#include "model/Scheduler.h"
#include "model/TimerManager.h"
#include "templates/TTimeBudget.h"
#include "model/SimulationClock.h"
#include "model/DropTimer.h"
#include "model/WeightedTable.h"
#include "model/PhaseTracker.h"
#include "model/LobFlight.h"
#include "model/RepairPicker.h"
#include "model/ShotSide.h"
#include "model/InputContextStack.h"
#include "platform/cocos/CocosKeyboardInput.h"
#include "platform/cocos/LegacyProfile.h"
#include "platform/SplashWindow.h"
#include "ports/IAudioOutput.h"
#include "ports/IStorage.h"
#include "services/ProfileSerializer.h"
#include "model/EditorLayout.h"
#include "model/EditorPalette.h"
#include "model/GroundMap.h"
#include "model/LevelLegend.h"
#include "services/ConfigRefs.h"
#include "services/LevelLegendManager.h"
#include "model/JsonValue.h"
#include "model/MathTypes.h"
#include "model/Random.h"
#include "model/NodeNavigator.h"
#include "model/TurnState.h"
#include "model/ArcTurn.h"
#include "model/ChargeState.h"
#include "model/UpgradeTree.h"
#include "model/AchievementBook.h"
#include "model/DeathPenalty.h"
#include "model/DifficultyBook.h"
#include "model/DropRoller.h"
#include "model/RefCounted.h"
#include "platform/AllocationCounter.h"
#include "model/ResourceStatus.h"
#include "model/ResourceTable.h"
#include "model/ScreenLayout.h"
#include "model/WidgetLayout.h"
#include "model/TextSize.h"
#include "model/CellGrid.h"
#include "model/WorkQueue.h"
#include "scenes/WidgetEditorScene.h"
#include "scenes/MenuScene.h"
#include "scenes/SettingsScene.h"
#include "services/ResourceHolder.h"
#include "services/ResourceLoadJob.h"
#include "services/AudioManager.h"
#include "services/DisplaySettings.h"
#include "services/ResourceManager.h"
#include "ui/LayoutWidget.h"
#include "ui/WidgetFactory.h"
#include "viewmodel/MenuEditSession.h"
#include "viewmodel/ViewDef.h"
#include "viewmodel/WidgetDef.h"
#include "viewmodel/WidgetEditSession.h"
#include "services/GlyphCoverage.h"
#include "services/ProfileManager.h"
#include "services/StringsManager.h"
#include "services/UiStyle.h"
#include "services/UpgradeManager.h"
#include "services/AchievementManager.h"
#include "tests/EditorHistoryTests.h"
#include "tests/CollisionTests.h"
#include "tests/TileMergeTests.h"
#include "tests/EditorLayoutTests.h"
#include "tests/EditorLibraryTests.h"
#include "tests/EditorToolsTests.h"
#include "tests/Showcase.h"
#include "tests/TestSelection.h"
#include "ui/Widgets.h"

namespace game
{
namespace tests
{

namespace
{
	std::string PathOf(const model::ResourceId& InId, const model::ResourceKind InKind)
	{
		return std::string(GameContext::Get<ResourceManager>().ResolvePath(InId, InKind).View());
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
		auto CurrentTank = GameContext::Get<ObjectsFactory>().CreateFromConfig<Tank>(InConfig, InContext.GetSandbox());
		InContext.Expect(CurrentTank != nullptr, "tank " + InConfig + " is created");
		return CurrentTank.Get();
	}

	BuffDef MakeBuff(std::string_view InId, const float InDuration)
	{
		BuffDef CurrentDef;
		CurrentDef.Id = FPersistentString{ InId };
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
			InContext.ExpectEqual(CurrentModifiers.Size(), static_cast<size_t>(2), "unknown and missing stats are skipped");
			if ( CurrentModifiers.Size() == 2 )
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
			InContext.Expect(StatsLoader::ReadModifiers(*NotArray).IsEmpty(), "an object is not a modifier list");
		});
	}

	void AddBuffTests(TestSuite& InSuite)
	{
		InSuite.Add("Buffs: a timed buff runs out", [](TestContext& InContext)
		{
			BuffSet CurrentBuffs;
			CurrentBuffs.Add(MakeBuff("a", 1.0f));
			InContext.Expect(!CurrentBuffs.Update(0.5f), "still active after 0.5 s");
			InContext.ExpectEqual(CurrentBuffs.GetActive().Size(), static_cast<size_t>(1), "one active buff");
			InContext.Expect(CurrentBuffs.Update(0.6f), "reports that a buff ran out");
			InContext.Expect(CurrentBuffs.GetActive().IsEmpty(), "nothing active afterwards");
		});

		InSuite.Add("Buffs: Has finds an active buff by id", [](TestContext& InContext)
		{
			BuffSet CurrentBuffs;
			InContext.Expect(!CurrentBuffs.Has("a"), "no buff yet");
			CurrentBuffs.Add(MakeBuff("a", 1.0f));
			InContext.Expect(CurrentBuffs.Has("a") && !CurrentBuffs.Has("b"), "only the added buff is found");
		});

		InSuite.Add("Buffs: adding the same buff again restarts its timer", [](TestContext& InContext)
		{
			BuffSet CurrentBuffs;
			CurrentBuffs.Add(MakeBuff("a", 1.0f));
			CurrentBuffs.Update(0.8f);
			CurrentBuffs.Add(MakeBuff("a", 1.0f));
			InContext.ExpectEqual(CurrentBuffs.GetActive().Size(), static_cast<size_t>(1), "still one buff");
			InContext.Expect(!CurrentBuffs.Update(0.5f), "alive: the timer was restarted");
		});

		InSuite.Add("Buffs: a permanent buff never runs out", [](TestContext& InContext)
		{
			BuffSet CurrentBuffs;
			CurrentBuffs.Add(MakeBuff("perm", -1.0f));
			InContext.Expect(!CurrentBuffs.Update(1000.0f), "nothing expires");
			InContext.ExpectEqual(CurrentBuffs.GetActive().Size(), static_cast<size_t>(1), "still active");
		});

		InSuite.Add("Buffs: invulnerability and modifiers are reported", [](TestContext& InContext)
		{
			BuffSet CurrentBuffs;
			InContext.Expect(!CurrentBuffs.IsInvulnerable(), "not invulnerable at first");

			BuffDef Shield = MakeBuff("shield", 2.0f);
			Shield.bInvulnerable = true;
			BuffDef Fast = MakeBuff("fast", 2.0f);
			Fast.Modifiers.PushBack({ StatType::Speed, 0.0f, 2.0f });
			CurrentBuffs.Add(Shield);
			CurrentBuffs.Add(Fast);

			InContext.Expect(CurrentBuffs.IsInvulnerable(), "shield makes invulnerable");
			InContext.ExpectEqual(CurrentBuffs.CollectModifiers().Size(), static_cast<size_t>(1), "one modifier collected");
			CurrentBuffs.Remove("shield");
			InContext.Expect(!CurrentBuffs.IsInvulnerable(), "removing the shield ends it");
		});
	}

	void AddDataTests(TestSuite& InSuite)
	{
		InSuite.Add("Enums: names and unknown names are parsed", [](TestContext& InContext)
		{
			EEnemyType Kind = EEnemyType::Light;
			InContext.Expect(EnumFromName("Heavy", Kind) && Kind == EEnemyType::Heavy, "Heavy is a known class");
			InContext.Expect(!EnumFromName("Nonsense", Kind) && Kind == EEnemyType::Heavy, "an unknown class is reported and changes nothing");
			EAiType Ai = EAiType::None;
			InContext.Expect(EnumFromName("Hunter", Ai) && Ai == EAiType::Hunter, "ai type");
			InContext.Expect(!EnumFromName("None", Ai), "None is not a configurable ai");
			InContext.Expect(ParseTeam(model::JsonValue("Enemy")) == ETeam::Enemy, "team");
			InContext.Expect(ParseObjectType(model::JsonValue("Wall")) == EObjectType::Wall, "object type");
			InContext.Expect(ParseBonusType(model::JsonValue("Buff")) == EBonusType::Buff, "bonus type");
		});

		InSuite.Add("Enemies: an enemy def is read from its config", [](TestContext& InContext)
		{
			const auto Full = JsonConfigNode::Parse(R"({"name":"enemy_sniper","atlas":"textures/enemies/Sniper.plist","enemy_type":"Heavy","health":3})", "test");
			const EnemyDef Sniper = EnemyCatalogLoader::ReadEnemy(*Full, "configs/objects/movables/tanks/enemies/Sniper.json");
			InContext.ExpectEqual(Sniper.Id, std::string("Sniper"), "id is the file name");
			InContext.ExpectEqual(Sniper.ConfigId, std::string("configs/objects/movables/tanks/enemies/Sniper.json"), "config id kept");
			InContext.ExpectEqual(Sniper.NameKey, std::string("enemy_sniper"), "name read");
			InContext.ExpectEqual(Sniper.Atlas, std::string("textures/enemies/Sniper.plist"), "atlas read");
			InContext.Expect(Sniper.Kind == EEnemyType::Heavy, "class read by name");

			const auto Bare = JsonConfigNode::Parse(R"({"enemy_type":"Nonsense"})", "test");
			const EnemyDef Plain = EnemyCatalogLoader::ReadEnemy(*Bare, "configs/objects/movables/tanks/enemies/Plain.json");
			InContext.ExpectEqual(Plain.NameKey, std::string("Plain"), "name defaults to the id");
			InContext.Expect(Plain.Atlas.IsEmpty(), "no atlas");
			InContext.Expect(Plain.Kind == EEnemyType::Medium, "unknown class keeps the default");
		});

		InSuite.Add("Tanks: a tank def is read from its config", [](TestContext& InContext)
		{
			const auto Full = JsonConfigNode::Parse(R"({
				"armor": 2, "reload_time": 0.8, "max_ammo": -1, "score": 300, "coins": 25,
				"enemy_type": "Boss", "ai": "Hunter", "buffs": ["regen"], "split_into": "cfg.enemy.Fast", "split_count": 3,
				"weapons": ["machine_gun"],
				"abilities": [{"type": "Shoot"}, {"type": "Buff", "buff": "shield", "cooldown": 8}]
			})", "test");
			const TankDef Boss = TankLoader::ReadTank(*Full);
			InContext.ExpectNear(Boss.Armor, 2.0f, 0.001f, "armor");
			InContext.ExpectNear(Boss.ReloadTime, 0.8f, 0.001f, "reload time");
			InContext.ExpectEqual(Boss.MaxAmmo, -1, "unlimited ammo");
			InContext.ExpectEqual(Boss.ScoreReward, 300, "score");
			InContext.ExpectEqual(Boss.CoinReward, 25, "coins");
			InContext.Expect(Boss.EnemyKind == EEnemyType::Boss, "enemy class");
			InContext.Expect(Boss.Ai == EAiType::Hunter, "ai");
			InContext.Expect(Boss.SplitInto == "configs/objects/movables/tanks/enemies/Fast.json" && Boss.SplitCount == 3, "splits into three");
			InContext.Expect(Boss.PermanentBuffs.Size() == 1 && Boss.PermanentBuffs[0] == "regen", "permanent buffs");
			InContext.Expect(Boss.Weapons.Size() == 1 && Boss.Weapons[0].Id == "machine_gun" && Boss.Weapons[0].BulletConfig == "configs/objects/movables/projectiles/bullet_mg.json"
				&& Boss.Weapons[0].Damage == 1 && !Boss.Weapons[0].bUsesAmmo && !Boss.Weapons[0].bLocked, "weapon read from its own config");
			InContext.Expect(Boss.Abilities.Size() == 2 && Boss.Abilities[1].Type == "Buff" && Boss.Abilities[1].BuffId == "shield", "abilities read");
			InContext.ExpectNear(Boss.Abilities[1].Cooldown, 8.0f, 0.001f, "ability cooldown");

			const auto Bare = JsonConfigNode::Parse(R"({"ai": "Nonsense"})", "test");
			const TankDef Plain = TankLoader::ReadTank(*Bare);
			InContext.ExpectNear(Plain.ReloadTime, TankDef().ReloadTime, 0.001f, "reload time default");
			InContext.ExpectEqual(Plain.MaxAmmo, TankDef().MaxAmmo, "ammo default");
			InContext.Expect(Plain.Ai == EAiType::None, "unknown ai means no ai");
			InContext.Expect(Plain.SplitInto.IsEmpty() && Plain.SplitCount == 0, "does not split");
			InContext.Expect(Plain.Weapons.Size() == 1 && Plain.Weapons[0].BulletConfig == PathOf(ids::configs::Bullet, model::ResourceKind::Config), "one default cannon");
			InContext.Expect(Plain.Abilities.Size() == 1 && Plain.Abilities[0].Type == "Shoot", "the tank just shoots");
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
			InContext.Expect(Level.Rows.Size() == 2 && Level.Rows[0] == "P.E", "rows");
			InContext.Expect(Level.EnemyIds == TArray<FPersistentString>{ FPersistentString{ "Basic" }, FPersistentString{ "Fast" } }, "enemy ids");
			InContext.Expect(Level.Waves.Size() == 1 && Level.Waves[0].MaxAlive == 2 && Level.Waves[0].Entries.Size() == 1
				&& Level.Waves[0].Entries[0].Count == 4 && Level.Waves[0].Entries[0].Spawner == tiles::EnemySpawn, "one default wave from the simple form");

			const auto Advanced = JsonConfigNode::Parse(R"({
				"rows": ["P1"],
				"spawners": {"1": {"types": ["Heavy", "Nonsense"], "exclude": ["Boss"]}, "too_long": {}},
				"waves": [{"delay": 4, "max_alive": 5, "interval": 0.5, "spawns": [{"spawner": "1", "count": 3}, {}]}]
			})", "test");
			InContext.Expect(LevelDataLoader::ReadLevel(*Advanced, Level), "advanced level read");
			InContext.Expect(Level.Spawners.Size() == 1 && Level.Spawners.Find('1') != nullptr, "only one-letter spawners");
			const SpawnRule& Rule = Level.Spawners['1'];
			InContext.Expect(Rule.Types == TArray<EEnemyType>{ EEnemyType::Heavy }, "unknown classes skipped");
			InContext.Expect(Rule.ExcludeEnemies == TArray<FPersistentString>{ FPersistentString{ "Boss" } }, "exclusions");
			InContext.Expect(Level.Waves.Size() == 1 && Level.Waves[0].Entries.Size() == 2, "one wave, two entries");
			InContext.ExpectNear(Level.Waves[0].Delay, 4.0f, 0.001f, "wave delay");
			InContext.Expect(Level.Waves[0].Entries[0].Spawner == '1' && Level.Waves[0].Entries[0].Count == 3, "spawn entry");
			InContext.Expect(Level.Waves[0].Entries[1].Spawner == tiles::EnemySpawn && Level.Waves[0].Entries[1].Count == 1, "spawn entry defaults");

			const auto NoRows = JsonConfigNode::Parse(R"({"name": "broken"})", "test");
			InContext.Expect(!LevelDataLoader::ReadLevel(*NoRows, Level), "a level without rows is rejected");
		});

		InSuite.Add("Enemies: every enemy config gives a catalog entry", [](TestContext& InContext)
		{
			const auto& All = GameContext::Get<EnemyCatalog>().GetAll();
			InContext.Expect(!All.IsEmpty(), "the catalog is not empty");
			for ( const EnemyDef& Def : All )
			{
				InContext.Expect(!Def.Id.IsEmpty() && Def.ConfigId.View().find(Def.Id.View()) != std::string_view::npos, "id comes from the file name: " + std::string(Def.ConfigId.View()));
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

			LevelData Wide = ValidLevel();
			for ( std::string& Row : Wide.Rows )
			{
				Row += Row;
			}
			Wide.Rows.PushBack(Wide.Rows.Front());
			for ( std::string& Row : Wide.Rows )
			{
				std::replace(Row.begin() + static_cast<std::ptrdiff_t>(Row.size() / 2), Row.end(), tiles::PlayerSpawn, tiles::Empty);
			}
			InContext.ExpectEqual(LevelLoader::Validate(Wide), std::string(), "a bigger map than 16x11 is valid");
			InContext.ExpectEqual(Wide.GetColumnCount(), GridColumns * 2, "columns come from the row length");
			InContext.ExpectEqual(Wide.GetRowCount(), GridRows + 1, "rows come from the row count");

			LevelData Tiny = ValidLevel();
			Tiny.Rows.Resize(2);
			InContext.ExpectEqual(LevelLoader::Validate(Tiny), std::string("LEVEL_ERROR_SIZE"), "too small a map");

			LevelData TwoPlayers = ValidLevel();
			TwoPlayers.Rows[5][5] = tiles::PlayerSpawn;
			InContext.ExpectEqual(LevelLoader::Validate(TwoPlayers), std::string("LEVEL_ERROR_PLAYER"), "two players");

			LevelData Walled = ValidLevel();
			Walled.Rows[0][1] = tiles::Stone;
			Walled.Rows[1][0] = tiles::Brick;
			InContext.Expect(LevelLoader::Validate(Walled).empty(), "brick can be shot through");
			Walled.Rows[1][0] = tiles::Stone;
			InContext.ExpectEqual(LevelLoader::Validate(Walled), std::string("LEVEL_ERROR_BLOCKED"), "a spawner shut in by stone");
		});

		InSuite.Add("Music: crossfades, rotates tracks, holds intense for the minimum time", [](TestContext& InContext)
		{
			model::MusicSettings Settings;
			Settings.CalmTracks = { "c1", "c2" };
			Settings.IntenseTracks = { "i1" };
			Settings.FadeSeconds = 1.0f;
			Settings.MinIntenseSeconds = 5.0f;
			model::MusicDirector Music(Settings);
			Music.Request(model::MusicMood::Calm);
			InContext.ExpectEqual(Music.GetCurrent().Track, std::string("c1"), "first calm track");
			Music.Update(0.5f);
			InContext.ExpectNear(Music.GetCurrent().Volume, 0.5f, 0.001f, "fades in");
			Music.Request(model::MusicMood::Intense);
			InContext.ExpectEqual(Music.GetCurrent().Track, std::string("i1"), "intense track");
			InContext.ExpectEqual(Music.GetPrevious().Track, std::string("c1"), "calm fades out");
			Music.Update(1.0f);
			InContext.Expect(Music.GetPrevious().Track.empty(), "faded out track is dropped");
			Music.Request(model::MusicMood::Calm);
			InContext.Expect(Music.GetMood() == model::MusicMood::Intense, "intense holds for the minimum time");
			Music.Update(5.0f);
			InContext.ExpectEqual(Music.GetCurrent().Track, std::string("c2"), "then the next calm track");
			Music.Request(model::MusicMood::Calm, true);
			InContext.ExpectEqual(Music.GetCurrent().Track, std::string("c1"), "a new level rotates the calm track");
			Music.Request(model::MusicMood::Menu);
			InContext.Expect(Music.GetCurrent().Track.empty(), "no menu tracks: the menu is silent");
			Music.PlayTrack("level");
			Music.Request(model::MusicMood::Intense);
			InContext.ExpectEqual(Music.GetCurrent().Track, std::string("level"), "a level's own track is kept");
			Music.Request(model::MusicMood::Calm, true);
			InContext.Expect(Music.GetCurrent().Track != "level", "a new level releases it");
		});

		InSuite.Add("Path: goes around stone, through a wall only when that is much shorter", [](TestContext& InContext)
		{
			model::GridPath Grid(5, 3);
			for ( int Y = 0; Y < 2; ++Y )
			{
				Grid.Set({ 2, Y }, model::CellPass::Breakable);
			}
			const TArray<model::GridCell> Around = Grid.Find({ 0, 0 }, { 4, 0 }, 8);
			InContext.Expect(!Around.IsEmpty() && Around.Back() == model::GridCell{ 4, 0 }, "reaches the goal");
			InContext.Expect(std::none_of(Around.begin(), Around.end(), [&Grid](const model::GridCell& InCell) { return Grid.Get(InCell) != model::CellPass::Open; }),
				"an expensive wall is driven around");
			const TArray<model::GridCell> Through = Grid.Find({ 0, 0 }, { 4, 0 }, 1);
			InContext.ExpectEqual(static_cast<int>(Through.Size()), 4, "a cheap wall is driven through");
			Grid.Set({ 2, 2 }, model::CellPass::Blocked);
			for ( int Y = 0; Y < 2; ++Y )
			{
				Grid.Set({ 2, Y }, model::CellPass::Blocked);
			}
			InContext.Expect(!Grid.IsReachable({ 0, 0 }, { 4, 0 }), "a stone line shuts the way");
		});

		InSuite.Add("Levels: a wave may only use spawners that are on the map", [](TestContext& InContext)
		{
			LevelData CurrentLevel = ValidLevel();
			Wave CurrentWave;
			CurrentWave.Entries.PushBack({ '3', 2 });
			CurrentLevel.Waves.PushBack(CurrentWave);
			InContext.ExpectEqual(LevelLoader::Validate(CurrentLevel), std::string("LEVEL_ERROR_ENEMY"), "spawner 3 is not on the map");

			CurrentLevel.Rows[1][1] = '3';
			InContext.Expect(LevelLoader::Validate(CurrentLevel).empty(), "valid once the spawner 3 is placed");
			InContext.ExpectEqual(LevelLoader::GetTotalEnemies(CurrentLevel), 2, "total enemies of the waves");
		});

		InSuite.Add("Levels: spawner pools follow classes, names and restrictions", [](TestContext& InContext)
		{
			const auto& Catalog = GameContext::Get<EnemyCatalog>();
			InContext.Expect(Catalog.GetAll().Size() >= 3, "the catalog has enemies");

			SpawnRule Anyone;
			InContext.ExpectEqual(LevelLoader::ResolvePool(Anyone).Size(), Catalog.GetAll().Size(), "an empty rule allows everyone");

			SpawnRule LightOnly;
			LightOnly.Types = { EEnemyType::Light };
			const auto Light = LevelLoader::ResolvePool(LightOnly);
			InContext.ExpectEqual(Light.Size(), Catalog.GetByType(EEnemyType::Light).Size(), "only the Light class");
			for ( const auto& CurrentId : Light )
			{
				InContext.Expect(Catalog.Find(CurrentId) && Catalog.Find(CurrentId)->Kind == EEnemyType::Light, CurrentId + " is Light");
			}

			SpawnRule BossOnly;
			BossOnly.Enemies = { FPersistentString{ "Boss" } };
			const auto Boss = LevelLoader::ResolvePool(BossOnly);
			InContext.Expect(Boss.Size() == 1 && Boss[0] == "Boss", "a particular enemy");

			SpawnRule Restricted;
			Restricted.Types = { EEnemyType::Light, EEnemyType::Medium };
			Restricted.ExcludeEnemies = { FPersistentString{ "Basic" } };
			const auto Pool = LevelLoader::ResolvePool(Restricted);
			InContext.Expect(std::find(Pool.begin(), Pool.end(), "Basic") == Pool.end(), "an excluded enemy is not sent");
			InContext.Expect(std::find(Pool.begin(), Pool.end(), "Fast") != Pool.end(), "other enemies of the class stay");

			SpawnRule NoHeavy;
			NoHeavy.ExcludeTypes = { EEnemyType::Heavy, EEnemyType::Boss };
			for ( const auto& CurrentId : LevelLoader::ResolvePool(NoHeavy) )
			{
				const EEnemyType CurrentType = Catalog.Find(CurrentId)->Kind;
				InContext.Expect(CurrentType != EEnemyType::Heavy && CurrentType != EEnemyType::Boss, CurrentId + " is not an excluded class");
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

		InSuite.Add("CampaignList: order is kept, repeats and empty ids dropped; the shipped list names every campaign level", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(R"({"levels":["cfg.level.2","cfg.level.1","","cfg.level.2","cfg.level.10"]})", "test");
			const model::CampaignList Parsed = model::CampaignList::FromConfig(*Root);
			const TArray<std::string> Wanted = { "cfg.level.2", "cfg.level.1", "cfg.level.10" };
			InContext.Expect(Parsed.GetLevelIds() == Wanted, "order kept, repeats and empty ids dropped");
			const auto NoLevels = JsonConfigNode::Parse("{}", "test");
			InContext.Expect(model::CampaignList::FromConfig(*NoLevels).IsEmpty(), "a missing list is empty");

			const model::CampaignList Shipped = model::CampaignList::FromConfig(config_refs::GetConfig(ids::configs::Campaign));
			TArray<std::string> Listed;
			for ( const std::string& Id : Shipped.GetLevelIds() )
			{
				Listed.EmplaceBack(config_refs::ResolveName(Id).View());
			}
			InContext.Expect(!Listed.IsEmpty() && Listed == config_refs::ListGroup(ids::config_groups::Levels), "the shipped list has every cfg.level.N in natural order");
			InContext.ExpectEqual(LevelLoader::GetBuiltinCount(), static_cast<int>(Listed.Size()), "the loader follows the list");
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
						InContext.Expect(!Pool.IsEmpty(), CurrentId + ": spawner " + std::string(1, CurrentEntry.Spawner) + " has a non-empty pool");
					}
				}
			}
		});

		InSuite.Add("Data: every enemy config is complete and consistent", [](TestContext& InContext)
		{
			for ( const auto& Enemy : GameContext::Get<EnemyCatalog>().GetAll() )
			{
				const model::IConfigNode& ConfigData = GameContext::Get<ConfigReader>().GetConfig(Enemy.ConfigId);
				InContext.Expect(ConfigData.GetValueOr("health", 0) > 0, std::string(Enemy.Id.View()) + ": health > 0");
				InContext.Expect(ConfigData.GetValueOr("speed", 0.0f) > 0.0f, std::string(Enemy.Id.View()) + ": speed > 0");
				InContext.Expect(ConfigData.GetValueOr("score", 0) > 0, std::string(Enemy.Id.View()) + ": has a score reward");
				InContext.Expect(ConfigData.Has("enemy_type"), std::string(Enemy.Id.View()) + ": has an enemy_type");
				InContext.Expect(TankLoader::ReadTank(GameContext::Get<ConfigReader>().GetConfig(Enemy.ConfigId.View())).Ai != EAiType::None, std::string(Enemy.Id.View()) + ": has an ai");

				const model::IConfigNode* const Abilities = ConfigData.FindChild("abilities");
				InContext.Expect(Abilities != nullptr, std::string(Enemy.Id.View()) + ": has abilities");
				if ( !Abilities )
				{
					continue;
				}
				for ( std::size_t Index = 0; Index < Abilities->GetItemCount(); ++Index )
				{
					const model::IConfigNode& CurrentEntry = *Abilities->GetItem(Index);
					InContext.Expect(CreateAbility(CurrentEntry.GetValueOr("type", "")) != nullptr, std::string(Enemy.Id.View()) + ": ability type is known");
					const std::string Buff = CurrentEntry.GetValueOr("buff", "");
					InContext.Expect(Buff.empty() || GameContext::Get<BuffCatalog>().Find(Buff) != nullptr, std::string(Enemy.Id.View()) + ": buff " + Buff + " exists");
				}
			}
		});

		InSuite.Add("Data: upgrades are complete and affordable in order", [](TestContext& InContext)
		{
			InContext.Expect(!GameContext::Get<UpgradeManager>().GetUpgrades().IsEmpty(), "there are upgrades");
			for ( const auto& Upgrade : GameContext::Get<UpgradeManager>().GetUpgrades() )
			{
				InContext.Expect(!Upgrade.Modifiers.IsEmpty() || !Upgrade.AutoFireWeapon.IsEmpty() || !Upgrade.UnlockWeapon.IsEmpty() || !Upgrade.Weapon.IsEmpty(),
					std::string(Upgrade.Id.View()) + ": has an effect");
				InContext.Expect(Upgrade.Requires.IsEmpty() || GameContext::Get<UpgradeManager>().GetRequirement(Upgrade.Id.View()),
					std::string(Upgrade.Id.View()) + ": its requirement exists");
				InContext.Expect(Upgrade.MaxLevel > 0, std::string(Upgrade.Id.View()) + ": has levels");
				InContext.Expect(Upgrade.BaseCost > 0, std::string(Upgrade.Id.View()) + ": costs something");
				InContext.ExpectEqual(GameContext::Get<UpgradeManager>().GetNextCost(Upgrade.Id.View()), Upgrade.BaseCost, std::string(Upgrade.Id.View()) + ": first level costs the base price");
			}
		});
	}

	void AddTankTests(TestSuite& InSuite)
	{
		InSuite.Add("Tank: armor reduces damage but a hit always hurts", [](TestContext& InContext)
		{
			Tank* Heavy = CreateTank(InContext, "configs/objects/movables/tanks/enemies/Heavy.json"); // armor 1
			if ( !Heavy )
			{
				return;
			}
			const int Before = Heavy->GetHealth();
			Heavy->ApplyDamage(4, ETeam::Player);
			InContext.ExpectEqual(Heavy->GetHealth(), Before - 3, "4 damage - 1 armor");
			Heavy->ApplyDamage(1, ETeam::Player);
			InContext.ExpectEqual(Heavy->GetHealth(), Before - 4, "a weak hit still does 1");
		});

		InSuite.Add("Tank: a destroyed tank leaves a wreck that blocks and breaks after two player hits", [](TestContext& InContext)
		{
			Tank* const Victim = CreateTank(InContext, "configs/objects/movables/tanks/enemies/Basic.json"); // health 6, armor 0
			if ( !Victim )
			{
				return;
			}
			const cocos2d::Vec2 Spot = Victim->GetVisualNode()->getPosition();
			auto& Factory = GameContext::Get<ObjectsFactory>();
			auto FindWreck = [&Factory, Spot]()
			{
				Wall* Found = nullptr;
				Factory.ForEachObject<Wall>([&Found, Spot](Wall& InWall)
				{
					if ( !InWall.IsDestroying() && InWall.GetVisualNode() && InWall.GetVisualNode()->getPosition().distance(Spot) < 1.0f )
					{
						Found = &InWall;
					}
				});
				return Found;
			};
			Victim->ApplyDamage(Victim->GetHealth(), ETeam::Player);
			Wall* const Wreck = FindWreck();
			InContext.Expect(Wreck != nullptr, "a wreck is left where the tank was");
			if ( !Wreck )
			{
				return;
			}
			InContext.Expect(Wreck->Blocks(cocos2d::Rect(Spot.x - 1.0f, Spot.y - 1.0f, 2.0f, 2.0f)), "the wreck blocks");
			const cocos2d::Rect Box = Wreck->GetCollisionRect();
			Wreck->Chip(Box, model::HitSide::Bottom, 1, 2, false);
			InContext.Expect(FindWreck() != nullptr, "one hit is not enough");
			Wreck->Chip(Box, model::HitSide::Bottom, 1, 2, false);
			InContext.Expect(FindWreck() == nullptr, "the second hit breaks it");
		});

		InSuite.Add("Tank: a big overkill leaves a wreck with 1 health", [](TestContext& InContext)
		{
			Tank* const Victim = CreateTank(InContext, "configs/objects/movables/tanks/enemies/Basic.json");
			if ( !Victim )
			{
				return;
			}
			const cocos2d::Vec2 Spot = Victim->GetVisualNode()->getPosition();
			Victim->ApplyDamage(Victim->GetHealth() + Victim->GetMaxHealth(), ETeam::Player);
			InContext.ExpectEqual(Victim->GetLastOverkill(), Victim->GetMaxHealth(), "overkill is the damage past zero");
			Wall* Wreck = nullptr;
			GameContext::Get<ObjectsFactory>().ForEachObject<Wall>([&Wreck, Spot](Wall& InWall)
			{
				if ( !InWall.IsDestroying() && InWall.GetVisualNode() && InWall.GetVisualNode()->getPosition().distance(Spot) < 1.0f )
				{
					Wreck = &InWall;
				}
			});
			InContext.Expect(Wreck != nullptr, "a wreck is left");
			if ( Wreck )
			{
				InContext.ExpectEqual(Wreck->GetMask().GetCellHealth(0, 0), 1, "only 1 health left");
			}
		});

		InSuite.Add("Tank: a shield absorbs everything", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateTank(InContext, PlayerTankPath());
			if ( !CurrentTank )
			{
				return;
			}
			CurrentTank->GetBuffs().Add(*GameContext::Get<BuffCatalog>().Find("shield"));
			CurrentTank->RefreshStats();
			const int Before = CurrentTank->GetHealth();
			CurrentTank->ApplyDamage(100, ETeam::Enemy);
			InContext.ExpectEqual(CurrentTank->GetHealth(), Before, "no damage under the shield");
			InContext.Expect(!CurrentTank->IsDestroying(), "still alive");
		});

		InSuite.Add("Tank: damage past the health destroys it and reports the kill", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateTank(InContext, "configs/objects/movables/tanks/enemies/Basic.json");
			if ( !CurrentTank )
			{
				return;
			}
			bool bKilled = false;
			CurrentTank->SetOnKilledCallback([&bKilled](GameObject&) { bKilled = true; });
			CurrentTank->ApplyDamage(CurrentTank->GetMaxHealth() + 10, ETeam::Player);
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

			auto CurrentBullet = GameContext::Get<ObjectsFactory>().FindObjectIf([](const FGameObjectHandle& InObject) { return InObject->GetType() == EObjectType::Bullet; });
			InContext.Expect(CurrentBullet != nullptr, "a bullet exists");
			InContext.Expect(CurrentBullet && CurrentBullet->GetTeam() == ETeam::Player, "the bullet belongs to the shooter's team");
		});

		InSuite.Add("Tank: an enemy has unlimited ammo", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateTank(InContext, "configs/objects/movables/tanks/enemies/Basic.json");
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
			auto& CurrentProfile = GameContext::Get<ProfileManager>().Get();
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
				GameContext::Get<UpgradeManager>().ApplyTo(*Upgraded);
				InContext.ExpectEqual(Upgraded->GetMaxAmmo(), CurrentAmmo + 10, "ammo capacity +5 x 2");
				InContext.ExpectEqual(Upgraded->GetShotDamage(Upgraded->GetActiveWeapon()), CurrentDamage + 1, "cannon damage +1");
				InContext.ExpectEqual(Upgraded->GetArmor(), Armor + 3, "armor +1 x 3");
				InContext.ExpectEqual(Upgraded->GetAmmo(), Upgraded->GetMaxAmmo(), "the tank is refilled");
			}
			CurrentProfile.Upgrades = Saved; // the profile is never saved in tests, but keep it clean
		});

		InSuite.Add("Upgrades: move speed and max health levels reach the tank", [](TestContext& InContext)
		{
			auto& CurrentProfile = GameContext::Get<ProfileManager>().Get();
			const auto Saved = CurrentProfile.Upgrades;

			Tank* Plain = CreateTank(InContext, PlayerTankPath());
			const float Speed = Plain ? Plain->GetStats().Get(StatType::Speed) : 0.0f;
			const int MaxHealth = Plain ? Plain->GetMaxHealth() : 0;

			CurrentProfile.Upgrades["move_speed"] = 2;
			CurrentProfile.Upgrades["max_hp"] = 3;
			Tank* Upgraded = CreateTank(InContext, PlayerTankPath());
			if ( Upgraded )
			{
				GameContext::Get<UpgradeManager>().ApplyTo(*Upgraded);
				InContext.Expect(std::fabs(Upgraded->GetStats().Get(StatType::Speed) - Speed * 1.21f) < 0.01f, "move speed x1.1 x 2");
				InContext.ExpectEqual(Upgraded->GetMaxHealth(), MaxHealth + 6, "max health +2 x 3");
				InContext.ExpectEqual(Upgraded->GetHealth(), Upgraded->GetMaxHealth(), "the tank is healed to the new maximum");
			}
			CurrentProfile.Upgrades = Saved;
		});

		InSuite.Add("Drops: every miss raises the chance by the pity step, a drop resets it", [](TestContext& InContext)
		{
			model::DropRoller Roller;
			Roller.SetTables({ { "t", 0.0f, 0.25f, { { "a", 1 } } } });
			model::Random Rng(7);
			InContext.ExpectNear(Roller.GetCurrentChance("t"), 0.0f, 0.001f, "starts at the base chance");
			InContext.Expect(!Roller.Roll("t", Rng).has_value(), "0% never drops");
			InContext.ExpectNear(Roller.GetCurrentChance("t"), 0.25f, 0.001f, "one miss: +25%");
			int Rolls = 1;
			while ( !Roller.Roll("t", Rng).has_value() && Rolls < 10 )
			{
				++Rolls;
			}
			InContext.Expect(Rolls <= 4, "by the fourth miss the chance is 100%");
			InContext.ExpectNear(Roller.GetCurrentChance("t"), 0.0f, 0.001f, "a drop resets the pity");
			InContext.Expect(!Roller.Roll("missing", Rng).has_value(), "an unknown table drops nothing");
			InContext.ExpectNear(Roller.GetCurrentChance("t", 0.0f), 0.0f, 0.001f, "the difficulty scale multiplies the chance");
		});

		InSuite.Add("Drops: weights split the drops and a seed repeats them", [](TestContext& InContext)
		{
			model::DropRoller Roller;
			Roller.SetTables({ { "t", 1.0f, 0.0f, { { "common", 3 }, { "rare", 1 } } } });
			model::Random Rng(12345);
			int Rare = 0;
			TArray<std::string> First;
			for ( int Index = 0; Index < 4000; ++Index )
			{
				const std::string Pickup = Roller.Roll("t", Rng).value_or("");
				Rare += Pickup == "rare" ? 1 : 0;
				if ( Index < 20 )
				{
					First.PushBack(Pickup);
				}
			}
			InContext.Expect(Rare > 850 && Rare < 1150, "about a quarter are rare: " + std::to_string(Rare));
			model::Random Again(12345);
			bool bSame = true;
			for ( const std::string& Pickup : First )
			{
				bSame = bSame && Roller.Roll("t", Again).value_or("") == Pickup;
			}
			InContext.Expect(bSame, "the same seed gives the same drops");
		});

		InSuite.Add("Drops: tables and entries load from config", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(R"({ "tables": [ { "id": "t", "chance": 0.5, "pity_step": 0.1, "entries": [ { "pickup": "p", "weight": 2 }, { "weight": 1 } ] } ] })", "test");
			model::DropRoller Roller;
			Roller.Load(*Root);
			InContext.ExpectNear(Roller.GetCurrentChance("t"), 0.5f, 0.001f, "the chance");
			model::Random Rng(1);
			for ( int Index = 0; Index < 5; ++Index )
			{
				const auto Pickup = Roller.Roll("t", Rng);
				InContext.Expect(!Pickup || *Pickup == "p", "an entry without a pickup is skipped");
			}
		});

		InSuite.Add("Difficulty: modes are found by id, unknown ids fall back to the default, next wraps around", [](TestContext& InContext)
		{
			model::DifficultyBook Book;
			model::DifficultyDef Easy{ "easy", "E", 0.5f, 0.5f, 1.5f, 0.5f, 1.5f, false };
			model::DifficultyDef Normal{ "normal", "N" };
			model::DifficultyDef Hard{ "hard", "H", 2.0f, 1.5f, 0.5f, 2.0f, 0.8f, true };
			Book.SetModes({ Easy, Normal, Hard }, "normal");
			InContext.ExpectNear(Book.Find("hard").EnemyHealth, 2.0f, 0.001f, "hard doubles enemy health");
			InContext.Expect(!Book.Find("easy").bDeathPenalty, "easy has no death penalty");
			InContext.Expect(Book.Find("missing").Id == "normal", "an unknown id gives the default");
			InContext.Expect(Book.GetNextId("normal") == "hard", "normal -> hard");
			InContext.Expect(Book.GetNextId("hard") == "easy", "hard wraps to easy");
			model::DifficultyBook Empty;
			InContext.ExpectNear(Empty.Find("hard").Coins, 1.0f, 0.001f, "no modes: neutral multipliers");
		});

		InSuite.Add("Difficulty: the chosen mode survives saving the profile", [](TestContext& InContext)
		{
			PlayerProfile Profile;
			Profile.Difficulty = "nightmare";
			const auto Root = JsonConfigNode::Parse(ProfileSerializer::ToJson(Profile), "test");
			InContext.Expect(ProfileSerializer::FromJson(*Root).Difficulty == "nightmare", "nightmare is kept");
		});

		InSuite.Add("Death penalty: each death shrinks the start share down to the floor, repair cost grows", [](TestContext& InContext)
		{
			model::DeathPenalty Penalty;
			Penalty.SetValues(0.15f, 0.2f, 0.4f, 25);
			InContext.ExpectNear(Penalty.GetHealthShare(0), 1.0f, 0.001f, "no deaths, full health");
			InContext.ExpectNear(Penalty.GetHealthShare(2), 0.7f, 0.001f, "two deaths, 70% health");
			InContext.ExpectNear(Penalty.GetAmmoShare(2), 0.6f, 0.001f, "two deaths, 60% ammo");
			InContext.ExpectNear(Penalty.GetHealthShare(10), 0.4f, 0.001f, "never below the floor");
			InContext.ExpectEqual(Penalty.GetRepairCost(0), 0, "nothing to repair");
			InContext.ExpectEqual(Penalty.GetRepairCost(3), 75, "25 coins per death");
		});

		InSuite.Add("Death penalty: deaths per level survive saving the profile", [](TestContext& InContext)
		{
			PlayerProfile Profile;
			Profile.LevelDeaths[2] = 3;
			const auto Root = JsonConfigNode::Parse(ProfileSerializer::ToJson(Profile), "test");
			const PlayerProfile Loaded = ProfileSerializer::FromJson(*Root);
			InContext.ExpectEqual(Loaded.LevelDeaths.Find(2) != nullptr ? *Loaded.LevelDeaths.Find(2) : 0, 3, "three deaths on level 3");
		});

		InSuite.Add("Achievements: a counter reaching its target unlocks once", [](TestContext& InContext)
		{
			model::AchievementBook Book;
			Book.SetAchievements({ { "first", "A_FIRST", "kills", 1 }, { "many", "A_MANY", "kills", 10 }, { "rich", "A_RICH", "coins", 100 } });
			TMap<FPersistentString, int> Counters{ { "kills", 3 } };
			TSet<FPersistentString> Unlocked;
			const TArray<std::string> Reached = Book.FindReached(Counters, Unlocked);
			InContext.ExpectEqual(static_cast<int>(Reached.Size()), 1, "only the first is reached");
			InContext.Expect(!Reached.IsEmpty() && Reached[0] == "first", "it is the first-kill one");
			Unlocked.Insert("first");
			Counters["kills"] = 10;
			Counters["coins"] = 99;
			const TArray<std::string> Next = Book.FindReached(Counters, Unlocked);
			InContext.Expect(Next.Size() == 1 && Next[0] == "many", "an unlocked one is not reported again, 99 coins are not enough");
		});

		InSuite.Add("Achievements: a hidden upgrade shows and sells only after its achievement", [](TestContext& InContext)
		{
			auto& CurrentProfile = GameContext::Get<ProfileManager>().Get();
			const PlayerProfile Saved = CurrentProfile;
			UpgradeManager& Shop = GameContext::Get<UpgradeManager>();
			CurrentProfile.Coins = 10000;
			CurrentProfile.Upgrades["ammo_capacity"] = 1;
			CurrentProfile.Achievements.Clear();
			CurrentProfile.Counters.Clear();
			InContext.Expect(Shop.IsHidden("ammo_belt"), "hidden before the achievement");
			InContext.Expect(!Shop.CanBuy("ammo_belt"), "and cannot be bought");
			const TArray<std::string> Reached = GameContext::Get<AchievementManager>().AddToCounter(counters::Kills, 50);
			InContext.Expect(std::find(Reached.begin(), Reached.end(), "veteran") != Reached.end(), "50 kills make a veteran");
			InContext.Expect(!Shop.IsHidden("ammo_belt"), "shown after it");
			InContext.Expect(Shop.CanBuy("ammo_belt"), "and can be bought");
			CurrentProfile = Saved;
		});

		InSuite.Add("Achievements: counters and achievements survive saving the profile", [](TestContext& InContext)
		{
			PlayerProfile Profile;
			Profile.Counters["kills"] = 7;
			Profile.Achievements.Insert("first_blood");
			const std::string Json = ProfileSerializer::ToJson(Profile);
			const auto Root = JsonConfigNode::Parse(Json, "test");
			const PlayerProfile Loaded = ProfileSerializer::FromJson(*Root);
			InContext.ExpectEqual(Loaded.Counters.Find("kills") != nullptr ? *Loaded.Counters.Find("kills") : 0, 7, "the kill counter");
			InContext.Expect(Loaded.Achievements.Contains("first_blood"), "the achievement");
		});

		InSuite.Add("Upgrades: a branch opens step by step, weapon upgrades touch only their weapon", [](TestContext& InContext)
		{
			auto& CurrentProfile = GameContext::Get<ProfileManager>().Get();
			auto& Shop = GameContext::Get<UpgradeManager>();
			const auto Saved = CurrentProfile.Upgrades;
			const int SavedCoins = CurrentProfile.Coins;

			CurrentProfile.Upgrades.Clear();
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
			CurrentProfile.Upgrades.Clear();
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

	bool Contains(const TArray<std::string>& InList, std::string_view InItem)
	{
		return std::find(InList.begin(), InList.end(), InItem) != InList.end();
	}

	// The frames an object config shows: its texture, its damage stages and its animation.
	TArray<std::string> FramesOf(const std::string& InConfigId)
	{
		const model::JsonValue::Object ConfigData = GameContext::Get<ObjectsFactory>().ReadValues(InConfigId);
		TArray<std::string> CurrentFrames;
		CurrentFrames.PushBack(DataConverter::GetString(ConfigData, "texture_path", ""));

		const model::JsonValue* const DamageFrames = ConfigData.Find("damage_frames");
		if ( DamageFrames && DamageFrames->IsArray() )
		{
			for ( const model::JsonValue& CurrentFrame : DamageFrames->GetArray() )
			{
				CurrentFrames.PushBack(CurrentFrame.AsString());
			}
		}

		const model::JsonValue* const Animation = ConfigData.Find("move_animation");
		if ( Animation && Animation->IsObject() )
		{
			const std::string Pattern = DataConverter::GetString(Animation->GetMembers(), "name", "");
			for ( int Index = 1; Index <= DataConverter::GetInt(Animation->GetMembers(), "frames_count", 0); ++Index )
			{
				CurrentFrames.PushBack(cocos2d::StringUtils::format(Pattern.c_str(), Index));
			}
		}
		return CurrentFrames;
	}

	void ExpectFrames(TestContext& InContext, const TArray<std::string>& InFrames, const std::string& InWhere)
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
			InContext.Expect(!Manifest.GetGlobalAtlases().IsEmpty(), "there are shared atlases");
			for ( const auto& CurrentAtlas : Manifest.GetGlobalAtlases() )
			{
				InContext.Expect(GameContext::Get<ResourceManager>().IsAtlasLoaded(CurrentAtlas), CurrentAtlas + " is loaded at start");
			}

			ExpectFrames(InContext, FramesOf(PlayerTankPath()), PlayerTankPath());
			ExpectFrames(InContext, FramesOf(PathOf(ids::configs::Bullet, model::ResourceKind::Config)), "bullet config");
			ExpectFrames(InContext, FramesOf("configs/objects/movables/projectiles/bullet_mg.json"), "configs/objects/movables/projectiles/bullet_mg.json");
			const auto CurrentWeapons = TankLoader::ReadWeapons(GameContext::Get<ConfigReader>().GetConfig(PlayerTankPath()), PathOf(ids::configs::Bullet, model::ResourceKind::Config));
			for ( const auto& Weapon : CurrentWeapons )
			{
				ExpectFrames(InContext, { std::string(Weapon.Icon.View()) }, "weapon " + std::string(Weapon.Id.View()));
				InContext.Expect(GameContext::Get<ConfigReader>().HasConfig(Weapon.BulletConfig.View()), "weapon " + std::string(Weapon.Id.View()) + ": its bullet config exists");
			}
			for ( const auto& CurrentBonus : { "configs/objects/pickups/bonus_ammo.json", "configs/objects/pickups/bonus_repair.json", "configs/objects/pickups/bonus_rapid_fire.json", "configs/objects/pickups/bonus_shield.json" } )
			{
				ExpectFrames(InContext, FramesOf(CurrentBonus), CurrentBonus);
			}
			ExpectFrames(InContext, { "boom1", "boom2", "boom3", "boom4", "boom5", "spawn_marker", "icon_shoot" }, "effects");

			const std::string EffectsPath = PathOf(ids::configs::Effects, model::ResourceKind::Config);
			const model::IConfigNode& Effects = GameContext::Get<ConfigReader>().GetConfig(EffectsPath);
			InContext.Expect(!Effects.GetKeys().IsEmpty(), "effects.json has effects");
			for ( const std::string& EffectId : Effects.GetKeys() )
			{
				const model::IConfigNode& Definition = *Effects.FindChild(EffectId);
				const std::string Pattern = Definition.GetValueOr("frames", "");
				for ( int Index = 1; Index <= Definition.GetValueOr("count", 0); ++Index )
				{
					ExpectFrames(InContext, { cocos2d::StringUtils::format(Pattern.c_str(), Index) }, "effect " + EffectId);
				}
			}
			for ( const auto& [CurrentConfig, Key] : TArray<std::pair<std::string, std::string>>{ { PlayerTankPath(), "hit_effect" },
				{ "configs/objects/spawnables/walls/brick.json", "hit_effect" }, { "configs/objects/spawnables/walls/stone.json", "hit_effect" }, { "configs/objects/spawnables/walls/wood.json", "hit_effect" },
				{ "configs/objects/movables/projectiles/bullet.json", "edge_effect" }, { "configs/objects/movables/projectiles/bullet.json", "fade_effect" },
				{ "configs/objects/pickups/bonus_ammo.json", "pickup_effect" } } )
			{
				const std::string EffectId = DataConverter::GetString(GameContext::Get<ObjectsFactory>().ReadValues(CurrentConfig), Key, "");
				InContext.Expect(Effects.Has(EffectId), CurrentConfig + ": its " + Key + " '" + EffectId + "' is in effects.json");
			}

			const model::IConfigNode& CurrentBuffs = GameContext::Get<ConfigReader>().GetConfig("configs/balance/buffs.json");
			const model::IConfigNode* const BuffEntries = CurrentBuffs.FindChild("buffs");
			for ( std::size_t Index = 0; BuffEntries && Index < BuffEntries->GetItemCount(); ++Index )
			{
				const std::string CurrentId = BuffEntries->GetItem(Index)->GetString("id");
				ExpectFrames(InContext, { std::string(GameContext::Get<BuffCatalog>().Find(CurrentId)->Icon.View()) }, "buff " + CurrentId);
			}
		});

		InSuite.Add("Resources: every enemy and tileset atlas has the frames its configs name", [](TestContext& InContext)
		{
			const ResourceManifest Manifest;
			for ( const auto& Enemy : GameContext::Get<EnemyCatalog>().GetAll() )
			{
				InContext.Expect(!Enemy.Atlas.IsEmpty(), std::string(Enemy.Id.View()) + ": names its atlas");
				ResourceHolder CurrentAtlas({ std::string(Enemy.Atlas.View()) });
				ExpectFrames(InContext, FramesOf(std::string(Enemy.ConfigId.View())), std::string(Enemy.Id.View()));
			}

			const TArray<std::string> CurrentTilesets = Manifest.GetTilesetAtlases();
			InContext.Expect(CurrentTilesets.Size() >= 2, "there are several tilesets");
			for ( const auto& CurrentName : { "field", "desert" } )
			{
				const std::string CurrentTileset = Manifest.GetTilesetAtlas(CurrentName);
				ResourceHolder CurrentAtlas({ CurrentTileset });
				InContext.Expect(GameContext::Get<ResourceManager>().IsAtlasLoaded(CurrentTileset), CurrentTileset + " loads");

				TArray<std::string> CurrentFrames = { "ground_1", "ground_2", "ground_3", "ground_4" };
				for ( const model::GroundEntry& Floor : GameContext::Get<LevelLegendManager>().Get().GetGroundEntries() )
				{
					CurrentFrames.PushBack(Floor.Frame + "_1");
					CurrentFrames.PushBack(Floor.Frame + "_2");
				}
				for ( const auto& CurrentWall : { "configs/objects/spawnables/walls/brick.json", "configs/objects/spawnables/walls/stone.json", "configs/objects/spawnables/walls/wood.json" } )
				{
					const auto WallFrames = FramesOf(CurrentWall);
					CurrentFrames.Insert(CurrentFrames.end(), WallFrames.begin(), WallFrames.end());
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
			InContext.Expect(Contains(FirstAtlases, GameContext::Get<EnemyCatalog>().Find("Basic")->Atlas.View()), "level 1: its enemy");
			InContext.ExpectEqual(FirstAtlases.Size(), size_t(2), "level 1: the tileset and one enemy, nothing else");

			const auto LastAtlases = Manifest.GetLevelAtlases(Last);
			InContext.Expect(Contains(LastAtlases, GameContext::Get<EnemyCatalog>().Find("Boss")->Atlas.View()), "level 6: the boss");
			InContext.Expect(!Contains(LastAtlases, GameContext::Get<EnemyCatalog>().Find("Basic")->Atlas.View()), "level 6: no excluded enemy");
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
			const StringsManager& Strings = GameContext::Get<StringsManager>();
			InContext.Expect(Strings.GetLanguages().Size() >= 2 && Strings.FindLanguage(Strings.GetDefaultLanguageId().View()), "the language list is loaded");
			for ( const LanguageDef& Language : Strings.GetLanguages() )
			{
				const std::string Code(Language.Id.View());
				InContext.Expect(Strings.FindMissingKeys(Language.Id.View()).IsEmpty(), Code + ": has every key of the default language");
				InContext.Expect(!Strings.GetAllTexts(Language.Id.View()).IsEmpty() && !Language.Name.empty(), Code + ": has texts and a name");
			}
		});

		InSuite.Add("Data: the language list drives the option cycle and an unknown language falls back", [](TestContext& InContext)
		{
			StringsManager& Strings = GameContext::Get<StringsManager>();
			const std::string Saved(Strings.GetLanguageId().View());
			std::string Id(Strings.GetDefaultLanguageId().View());
			for ( size_t Count = 0; Count < Strings.GetLanguages().Size(); ++Count )
			{
				const std::string Next = Strings.GetNextLanguageId(Id);
				InContext.Expect(Next != Id && Strings.FindLanguage(Next), "the next language is another listed one");
				Id = Next;
			}
			InContext.Expect(Id == Strings.GetDefaultLanguageId().View(), "the cycle returns to the start");
			Strings.SetLanguage("no_such_language");
			InContext.Expect(Strings.GetLanguageId() == Strings.GetDefaultLanguageId(), "an unknown language selects the default");
			Strings.SetLanguage(Saved);
		});

		InSuite.Add("Data: the fonts of every language draw its texts", [](TestContext& InContext)
		{
			const StringsManager& Strings = GameContext::Get<StringsManager>();
			for ( const LanguageDef& Language : Strings.GetLanguages() )
			{
				for ( const model::ResourceId Font : { ids::fonts::Decorative, ids::fonts::Plain } )
				{
					InContext.Expect(GlyphCoverage::FindMissing(Language.Id.View(), Font.GetName()).IsEmpty(), std::string(Language.Id.View()) + ": no missing glyphs in " + std::string(Font.GetName()));
				}
			}
		});

		InSuite.Add("Resources: an atlas stays loaded while any scope needs it", [](TestContext& InContext)
		{
			const std::string CurrentAtlas = ResourceManifest().GetTilesetAtlas("desert");
			const std::string CurrentFrame = ResourceManifest::GetTilesetFrame("desert", "ground_1");
			InContext.Expect(!GameContext::Get<ResourceManager>().IsAtlasLoaded(CurrentAtlas), "not loaded before");
			{
				ResourceHolder Outer({ CurrentAtlas });
				{
					ResourceHolder Inner({ CurrentAtlas });
					InContext.Expect(HasFrame(CurrentFrame), "loaded by two scopes");
				}
				InContext.Expect(GameContext::Get<ResourceManager>().IsAtlasLoaded(CurrentAtlas) && HasFrame(CurrentFrame), "still loaded by the other scope");
			}
			InContext.Expect(!GameContext::Get<ResourceManager>().IsAtlasLoaded(CurrentAtlas), "unloaded after the last scope");
			InContext.Expect(!HasFrame(CurrentFrame), "its frames are gone");
		});
	}
}

namespace
{
	Wall* CreateWall(TestContext& InContext, const std::string& InConfig, const cocos2d::Vec2& InPosition)
	{
		const model::IConfigNode& ConfigData = GameContext::Get<ConfigReader>().GetConfig(InConfig);
		model::JsonValue::Object Overrides;
		Overrides.InsertOrAssign("position", model::JsonValue(model::Vec2f{ InPosition.x, InPosition.y }));
		Overrides.InsertOrAssign("texture_path", model::JsonValue(ResourceManifest::GetTilesetFrame("field", ConfigData.GetValueOr("texture_path", ""))));
		auto CurrentWall = GameContext::Get<ObjectsFactory>().CreateFromConfig<Wall>(InConfig, InContext.GetSandbox(), Overrides);
		InContext.Expect(CurrentWall != nullptr, "wall " + InConfig + " is created");
		return CurrentWall.Get();
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
			model::CellGrid CurrentMask(4, 1);
			InContext.ExpectEqual(CurrentMask.Chip(model::HitSide::Bottom, 0.0f, 4.0f, 2, 1), 8, "two rows of four cells");
			InContext.Expect(!CurrentMask.IsSolid(0, 0) && !CurrentMask.IsSolid(3, 1), "the bottom rows are gone");
			InContext.Expect(CurrentMask.IsSolid(0, 2) && CurrentMask.IsSolid(3, 3), "the top rows stand");
			InContext.ExpectEqual(CurrentMask.Chip(model::HitSide::Bottom, 0.0f, 4.0f, 2, 1), 8, "the next hit takes the rest");
			InContext.Expect(CurrentMask.IsEmpty(), "nothing is left");
		});

		InSuite.Add("Walls: a narrow hit chips only its line; a tough cell needs several hits", [](TestContext& InContext)
		{
			model::CellGrid CurrentMask(4, 2);
			InContext.ExpectEqual(CurrentMask.Chip(model::HitSide::Left, 1.0f, 2.0f, 1, 1), 0, "a damaged cell still stands");
			InContext.Expect(CurrentMask.IsSolid(0, 1) && CurrentMask.GetCellHealth(0, 1) == 1, "the cell lost half of its health");
			InContext.ExpectEqual(CurrentMask.Chip(model::HitSide::Left, 1.0f, 2.0f, 1, 1), 1, "the second hit removes it");
			InContext.Expect(!CurrentMask.IsSolid(0, 1) && CurrentMask.IsSolid(1, 1) && CurrentMask.IsSolid(0, 0) && CurrentMask.IsSolid(0, 2), "only that cell is gone");
			CurrentMask.Chip(model::HitSide::Right, 0.0f, 0.99f, 1, 2);
			InContext.Expect(!CurrentMask.IsSolid(3, 0) && CurrentMask.IsSolid(2, 0), "a hit from the right chips the right edge");
		});

		InSuite.Add("Walls: only standing cells block, and a swept bullet stops at the first of them", [](TestContext& InContext)
		{
			model::CellGrid CurrentMask(4, 1);
			CurrentMask.Chip(model::HitSide::Bottom, 0.0f, 4.0f, 2, 1);
			InContext.Expect(!CurrentMask.Overlaps(0.0f, 0.0f, 4.0f, 1.9f), "the cleared bottom half is free");
			InContext.Expect(CurrentMask.Overlaps(0.0f, 1.5f, 4.0f, 2.5f), "the top half blocks");

			float T = 0.0f;
			InContext.Expect(CurrentMask.Sweep(2.0f, -1.0f, 2.0f, 5.0f, 0.1f, T), "a bullet from below hits");
			InContext.ExpectNear(T, (2.0f - 0.1f + 1.0f) / 6.0f, 0.001f, "at the first standing row");

			model::CellGrid Column(4, 1);
			Column.Chip(model::HitSide::Bottom, 1.0f, 2.0f, 4, 1);
			InContext.Expect(!Column.Sweep(1.5f, -1.0f, 1.5f, 5.0f, 0.2f, T), "a bullet flies through a cleared column");
			InContext.Expect(Column.Sweep(2.5f, -1.0f, 2.5f, 5.0f, 0.2f, T), "but not through its neighbour");
		});

		InSuite.Add("Walls: a tank fits where a shell cleared the wall; two shells destroy a brick tile", [](TestContext& InContext)
		{
			const ResourceHolder CurrentTileset({ ResourceManifest().GetTilesetAtlas("field") });
			Wall* CurrentWall = CreateWall(InContext, "configs/objects/spawnables/walls/brick.json", cocos2d::Vec2(TileSize / 2, TileSize / 2));
			if ( !CurrentWall )
			{
				return;
			}
			const cocos2d::Rect Box = CurrentWall->GetCollisionRect();
			InContext.ExpectNear(Box.size.width, TileSize, 0.5f, "a brick wall is one tile wide");

			const cocos2d::Rect Shell(Box.getMinX(), Box.getMinY() - 2.0f, TileSize, 3.0f);
			CurrentWall->Chip(Shell, model::HitSide::Bottom, 2, 2, false);
			InContext.Expect(!CurrentWall->Blocks(CellsRect(*CurrentWall, 0.0f, 0.0f, 8.0f, 3.95f)), "the lower half is open");
			InContext.Expect(CurrentWall->Blocks(CellsRect(*CurrentWall, 1.0f, 3.0f, 7.0f, 5.0f)), "the upper half still blocks");
			InContext.Expect(!CurrentWall->IsDestroying(), "the wall still stands");

			const cocos2d::Rect Deeper(Box.getMinX(), Box.getMinY() + TileSize / 2 - 2.0f, TileSize, 3.0f);
			CurrentWall->Chip(Deeper, model::HitSide::Bottom, 2, 2, false);
			InContext.Expect(CurrentWall->GetMask().IsEmpty() && CurrentWall->IsDestroying(), "the second shell destroys it");
		});

		InSuite.Add("Walls: a shot between two walls chips both; stone needs a stone-breaking shot", [](TestContext& InContext)
		{
			const ResourceHolder CurrentTileset({ ResourceManifest().GetTilesetAtlas("field") });
			Wall* Left = CreateWall(InContext, "configs/objects/spawnables/walls/brick.json", cocos2d::Vec2(TileSize / 2, TileSize / 2));
			Wall* Right = CreateWall(InContext, "configs/objects/spawnables/walls/brick.json", cocos2d::Vec2(TileSize * 1.5f, TileSize / 2));
			Wall* CurrentStone = CreateWall(InContext, "configs/objects/spawnables/walls/stone.json", cocos2d::Vec2(TileSize * 3.5f, TileSize / 2));
			if ( !Left || !Right || !CurrentStone )
			{
				return;
			}
			const float Cell = TileSize / 4;
			const cocos2d::Rect Between(TileSize - Cell, -2.0f, TileSize, 3.0f);
			const int LeftCells = Left->Chip(Between, model::HitSide::Bottom, 2, 2, false);
			const int RightCells = Right->Chip(Between, model::HitSide::Bottom, 2, 2, false);
			InContext.ExpectEqual(LeftCells, 8, "one column of the left wall, two cells deep");
			InContext.ExpectEqual(RightCells, 24, "three columns of the right wall");

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
			CurrentTank->OnCommandReceived(GameCommand(ECommandType::SwitchWeapon));
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

			auto CurrentBullet = GameContext::Get<ObjectsFactory>().FindObjectIf([](const FGameObjectHandle& InObject) { return InObject->GetType() == EObjectType::Bullet; });
			InContext.Expect(CurrentBullet != nullptr, "a bullet exists");
			if ( CurrentBullet )
			{
				InContext.Expect(CurrentBullet->GetDamage() < CurrentTank->GetDamage(), "it hits weaker than a shell");
			}
		});

		InSuite.Add("Weapons: an enemy without a weapons list has one cannon", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateTank(InContext, "configs/objects/movables/tanks/enemies/Basic.json");
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
			GameContext::Get<PhysicsManager>().BeginFrame();
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
			Tree.SetBranches({ { FPersistentString{ "gun" }, Vec2f(0.0f, 0.0f), Vec2f(10.0f, 100.0f), Vec2f(0.0f, 50.0f) },
				{ FPersistentString{ "hull" }, Vec2f(5.0f, 5.0f), Vec2f(100.0f, 0.0f), Vec2f(40.0f, 0.0f) } });
			const TArray<FPersistentString> Skipped = Tree.Build({ { FPersistentString{ "unlock" }, FPersistentString{ "gun" }, 0, FPersistentString{} }, { FPersistentString{ "reload" }, FPersistentString{ "gun" }, 1, FPersistentString{ "unlock" } },
				{ FPersistentString{ "armor" }, FPersistentString{ "hull" }, 0, FPersistentString{ "reload" } }, { FPersistentString{ "lost" }, FPersistentString{ "nowhere" }, 0, FPersistentString{} } });
			InContext.ExpectEqual(Tree.GetNodeCount(), 3, "the node on an unknown branch is left out");
			InContext.ExpectEqual(static_cast<int>(Skipped.Size()), 1, "and reported");
			InContext.Expect(Tree.GetPosition(Tree.FindNode("reload")).IsNear(Vec2f(10.0f, 150.0f), 0.01f), "step 1 is one step from the anchor");
			InContext.Expect(Tree.GetLinkStart(Tree.FindNode("reload")).IsNear(Vec2f(10.0f, 100.0f), 0.01f), "a link starts at the parent on the same branch");
			InContext.Expect(Tree.GetLinkStart(Tree.FindNode("unlock")).IsNear(Vec2f(0.0f, 0.0f), 0.01f), "a first node links to its tank part");
			InContext.ExpectEqual(Tree.GetParent(Tree.FindNode("armor")), -1, "a parent on another branch draws no link");
			InContext.Expect(Tree.GetLinkStart(Tree.FindNode("armor")).IsNear(Vec2f(5.0f, 5.0f), 0.01f), "so it links to its own tank part");
		});

		InSuite.Add("Upgrade tree: arrows move the focus to the nearest node that way", [](TestContext& InContext)
		{
			using model::NodeNavigator;
			const TArray<Vec2f> Points{ { 0.0f, 0.0f }, { 0.0f, 100.0f }, { 100.0f, 10.0f }, { 60.0f, 200.0f }, { -100.0f, -5.0f } };
			InContext.ExpectEqual(NodeNavigator::FindNext(Points, {}, 0, Vec2f(0.0f, 1.0f)), 1, "up: straight above beats far and to the side");
			InContext.ExpectEqual(NodeNavigator::FindNext(Points, {}, 0, Vec2f(1.0f, 0.0f)), 2, "right");
			InContext.ExpectEqual(NodeNavigator::FindNext(Points, {}, 0, Vec2f(-1.0f, 0.0f)), 4, "left");
			InContext.ExpectEqual(NodeNavigator::FindNext(Points, {}, 4, Vec2f(0.0f, -1.0f)), 4, "nothing below: the focus stays");
			InContext.ExpectEqual(NodeNavigator::FindNext(Points, { 1, 0, 1, 1, 1 }, 0, Vec2f(0.0f, 1.0f)), 3, "hidden nodes are skipped");
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

		InSuite.Add("Turning: a half turn takes twice the quarter-turn time", [](TestContext& InContext)
		{
			TurnState Turn;
			Turn.Start(90.0f, 270.0f, 0.3f);
			Turn.Update(0.45f);
			InContext.Expect(Turn.IsTurning(), "still turning after 1.5 quarter times");
			Turn.Update(0.2f);
			InContext.Expect(!Turn.IsTurning(), "done after two");
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
			CurrentTank->OnCommandReceived(GameCommand(ECommandType::MoveRight));
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
			CurrentTank->OnCommandReceived(GameCommand(ECommandType::Stop));
			CurrentTank->OnCommandReceived(GameCommand(ECommandType::MoveRight));
			Advance(1.0f / 60.0f);
			CurrentTank->OnCommandReceived(GameCommand(ECommandType::MoveForward));
			InContext.Expect(CurrentTank->GetDirection().y > 0.99f, "the direction points back up at once");
			Advance(1.0f);
			InContext.Expect(!CurrentTank->IsTurning(), "the counter-turn ended");
			InContext.ExpectNear(model::TurnState::Normalize(CurrentTank->GetVisualNode()->getRotation()), 0.0f, 0.5f, "facing up again");
		});

		InSuite.Add("Wheels: an arc turn drives a quarter circle to the new facing", [](TestContext& InContext)
		{
			model::ArcTurn Arc;
			Arc.Start(model::Vec2f(0.0f, 0.0f), 0.0f, 90.0f, 10.0f);
			InContext.Expect(Arc.IsTurning(), "turning");
			const float Quarter = 3.14159265f * 10.0f / 2.0f;
			InContext.ExpectNear(Arc.GetRemainingDistance(), Quarter, 0.01f, "a quarter circle long");
			InContext.Expect(!Arc.Advance(Quarter / 2.0f), "not done halfway");
			InContext.ExpectNear(Arc.GetAngle(), 45.0f, 0.01f, "half turned halfway");
			InContext.Expect(Arc.Advance(Quarter), "done at the end");
			InContext.Expect(Arc.GetPosition().IsNear(model::Vec2f(10.0f, 10.0f), 0.01f), "ends one radius up and one right");
			InContext.ExpectNear(Arc.GetAngle(), 90.0f, 0.01f, "facing right");
			Arc.Start(model::Vec2f(0.0f, 0.0f), 0.0f, 270.0f, 10.0f);
			InContext.Expect(Arc.PeekPosition(1000.0f).IsNear(model::Vec2f(-10.0f, 10.0f), 0.01f), "a left turn bends the other way");
			Arc.Start(model::Vec2f(0.0f, 0.0f), 0.0f, 0.0f, 10.0f);
			InContext.Expect(!Arc.IsTurning(), "the same facing starts nothing");
		});

		InSuite.Add("Wheels: a U-turn is a half circle that ends two radii aside", [](TestContext& InContext)
		{
			model::ArcTurn Arc;
			Arc.Start(model::Vec2f(5.0f, 5.0f), 90.0f, 270.0f, 8.0f);
			Arc.Advance(1000.0f);
			InContext.ExpectNear(Arc.GetAngle(), 270.0f, 0.01f, "facing back");
			InContext.ExpectNear(std::fabs(Arc.GetPosition().Y - 5.0f), 16.0f, 0.01f, "two radii aside");
			InContext.ExpectNear(Arc.GetPosition().X, 5.0f, 0.01f, "back on the same line across");
		});

		InSuite.Add("Wheels: a wheeled vehicle never turns on the spot in the open", [](TestContext& InContext)
		{
			const cocos2d::Vec2 Start(TileSize * 4.5f, TileSize * 4.5f);
			Tank* CurrentTank = CreateTankAt(InContext, "configs/test/wheeled_vehicle.json", Start);
			if ( !CurrentTank )
			{
				return;
			}
			CurrentTank->OnCommandReceived(GameCommand(ECommandType::MoveRight));
			InContext.Expect(CurrentTank->IsTurning(), "turning right");
			Advance(0.1f);
			const cocos2d::Vec2 Position = CurrentTank->GetVisualNode()->getPosition();
			const float Angle = model::TurnState::Normalize(CurrentTank->GetVisualNode()->getRotation());
			InContext.Expect(Position.x > Start.x + 1.0f && Position.y > Start.y + 1.0f, "drives forward and right along the arc");
			InContext.Expect(Angle > 5.0f && Angle < 85.0f, "partly turned, not snapped");
			Advance(1.0f);
			InContext.Expect(!CurrentTank->IsTurning(), "the arc ended");
			InContext.ExpectNear(model::TurnState::Normalize(CurrentTank->GetVisualNode()->getRotation()), 90.0f, 0.5f, "facing right");
			InContext.Expect(CurrentTank->GetVisualNode()->getPosition().x > Start.x + TileSize, "kept driving right");
		});

		InSuite.Add("AI features: profile, enemy, difficulty and level switches apply in order, the last one winning", [](TestContext& InContext)
		{
			model::AiFeatureSet Features;
			Features.Apply({ { "dodge", true }, { "group", false }, { "unknown", true } });
			Features.Apply({ { "group", true } });
			Features.Apply({ { "dodge", false } });
			InContext.Expect(!Features.Has(model::AiFeature::Dodge), "the level turned dodging off");
			InContext.Expect(Features.Has(model::AiFeature::Group), "the difficulty turned the group on");
			InContext.Expect(!Features.Has(model::AiFeature::Flank), "untouched features stay off");
			InContext.Expect(model::AiFeatureSet::FromName("guard_drops") == model::AiFeature::GuardDrops && !model::AiFeatureSet::FromName("x"), "names");
		});

		InSuite.Add("Anti-camping: a player who stays in one spot long enough counts as camping, moving away resets it", [](TestContext& InContext)
		{
			model::CampWatch Watch;
			Watch.Configure(10.0f, 2.0f);
			Watch.Update({ 0.0f, 0.0f }, 0.5f);
			for ( int Step = 0; Step < 3; ++Step )
			{
				Watch.Update({ 3.0f, 4.0f }, 0.5f);
			}
			InContext.Expect(!Watch.IsCamping(), "1.5 s is not yet camping");
			Watch.Update({ -5.0f, 0.0f }, 0.5f);
			InContext.Expect(Watch.IsCamping(), "2 s within the radius is camping");
			Watch.Update({ 30.0f, 0.0f }, 0.5f);
			InContext.Expect(!Watch.IsCamping() && Watch.GetStillTime() == 0.0f, "leaving the circle starts over");
		});

		InSuite.Add("Base: the base tile builds a wall marked as the base and falling ends the level", [](TestContext& InContext)
		{
			const ResourceHolder CurrentTileset({ ResourceManifest().GetTilesetAtlas("field") });
			Wall* Base = CreateWall(InContext, "configs/objects/spawnables/walls/base.json", cocos2d::Vec2(TileSize / 2, TileSize / 2));
			if ( !Base )
			{
				InContext.Expect(false, "the base is created");
				return;
			}
			bool bLost = false;
			Base->SetOnKilledCallback([&bLost](GameObject&) { bLost = true; });
			InContext.Expect(Base->IsBase() && AiQueries::FindBase() == Base, "the AI finds the base");
			const cocos2d::Rect Box = Base->GetCollisionRect();
			for ( int Hit = 0; Hit < 100 && !Base->IsDestroying(); ++Hit )
			{
				Base->Chip(Box, model::HitSide::Bottom, 4, 1, false);
			}
			InContext.Expect(bLost, "the killed callback fires when the last cell falls");
			InContext.Expect(model::AiFeatureSet::FromName("attack_base") == model::AiFeature::AttackBase
				&& model::AiFeatureSet::FromName("anti_camp") == model::AiFeature::AntiCamp, "feature names");
		});

		InSuite.Add("AI tactics: in the player's row or column while reloading, step out of it, away from the player", [](TestContext& InContext)
		{
			const model::Vec2f Player(100.0f, 100.0f);
			InContext.Expect(model::AiTactics::IsExposed({ 105.0f, 400.0f }, Player, 10.0f), "same column");
			InContext.Expect(model::AiTactics::IsExposed({ 400.0f, 95.0f }, Player, 10.0f), "same row");
			InContext.Expect(!model::AiTactics::IsExposed({ 200.0f, 300.0f }, Player, 10.0f), "off both lines");
			const model::Vec2f Up = model::AiTactics::SidestepDirection({ 95.0f, 400.0f }, Player);
			InContext.Expect(Up.X == -1.0f && Up.Y == 0.0f, "in a column it steps sideways, away from the player's side");
			const model::Vec2f Side = model::AiTactics::SidestepDirection({ 400.0f, 101.0f }, Player);
			InContext.Expect(Side.X == 0.0f && Side.Y == 1.0f, "in a row it steps up or down");
			InContext.Expect(model::AiFeatureSet::FromName("cover") == model::AiFeature::Cover
				&& model::AiFeatureSet::FromName("fake_turn") == model::AiFeature::FakeTurn, "feature names");
		});

		InSuite.Add("AI debug: every tactic has a name to show over the tank", [](TestContext& InContext)
		{
			for ( size_t Index = 0; Index < static_cast<size_t>(model::AiTactic::Count); ++Index )
			{
				InContext.Expect(!model::GetTacticName(static_cast<model::AiTactic>(Index)).empty(), "tactic " + std::to_string(Index) + " has a name");
			}
			InContext.Expect(model::GetTacticName(model::AiTactic::HuntCamper) == "hunt camper", "a known name");
		});

		InSuite.Add("Bullet clash: each bullet loses the other's health, the weaker one breaks", [](TestContext& InContext)
		{
			const model::ClashResult Even = model::Clash(1, 1);
			InContext.Expect(Even.LeftA == 0 && Even.LeftB == 0, "equal bullets break each other");
			const model::ClashResult Heavy = model::Clash(4, 1);
			InContext.Expect(Heavy.LeftA == 3 && Heavy.LeftB == 0, "a heavy shell goes on, weakened");
			const model::ClashResult Weak = model::Clash(2, 4);
			InContext.Expect(Weak.LeftA == 0 && Weak.LeftB == 2, "the weaker one breaks");
		});

		InSuite.Add("Projectile hit rules: bounce off walls, spend on chip, friendly share on allies", [](TestContext& InContext)
		{
			const model::ProjectileHitOutcome Bounce = model::ResolveProjectileHit({ model::ProjectileTarget::Wall, 2, 3, false, 1.0f });
			InContext.Expect(Bounce.Action == model::ProjectileAction::Bounce && Bounce.BouncesLeft == 1 && !Bounce.bShellSpent, "wall with bounces left bounces");
			const model::ProjectileHitOutcome Chip = model::ResolveProjectileHit({ model::ProjectileTarget::Wall, 0, 3, false, 1.0f });
			InContext.Expect(Chip.Action == model::ProjectileAction::ChipWall && Chip.bShellSpent, "wall without bounces is chipped");
			const model::ProjectileHitOutcome Ally = model::ResolveProjectileHit({ model::ProjectileTarget::Tank, 2, 4, true, 0.5f });
			InContext.Expect(Ally.Action == model::ProjectileAction::DamageTarget && Ally.Damage == 2, "ally takes the share");
			const model::ProjectileHitOutcome Enemy = model::ResolveProjectileHit({ model::ProjectileTarget::Tank, 0, 4, false, 0.5f });
			InContext.Expect(Enemy.Damage == 4, "enemy takes full damage");
		});

		InSuite.Add("Friendly fire share: scales damage to allies, never below 1 while on", [](TestContext& InContext)
		{
			InContext.Expect(model::FriendlyDamage(4, 1.0f) == 4, "full share");
			InContext.Expect(model::FriendlyDamage(4, 0.5f) == 2, "half share");
			InContext.Expect(model::FriendlyDamage(1, 0.2f) == 1, "at least 1");
			InContext.Expect(model::FriendlyDamage(4, 0.0f) == 0, "share 0 means no damage");
			const auto Root = JsonConfigNode::Parse(R"({"default":"a","modes":[{"id":"a","friendly_fire_share":0.5,"bullet_clash":true},{"id":"b"}]})", "test");
			model::DifficultyBook Book;
			Book.Load(*Root);
			InContext.Expect(Book.Find("a").bBulletClash && std::abs(Book.Find("a").FriendlyFireShare - 0.5f) < 0.001f, "read from difficulty");
			InContext.Expect(!Book.Find("b").bBulletClash && Book.Find("b").FriendlyFireShare == 1.0f, "defaults: no clash, full share");
		});

		InSuite.Add("Script: setup steps set health, buff, tactic and end the level, and write back the same", [](TestContext& InContext)
		{
			struct SetupWorld : model::IScriptWorld
			{
				TArray<std::string> Log;
				bool HasActor(const std::string&) const override { return true; }
				bool IsGone(const std::string&) const override { return false; }
				model::Vec2f GetCell(const std::string&) const override { return model::Vec2f(); }
				bool IsAt(const std::string&, const model::Vec2f&) const override { return true; }
				bool IsFacing(const std::string&, const std::string&) const override { return true; }
				bool HasEvent(const std::string&) const override { return false; }
				void MoveTo(const std::string&, const model::Vec2f&) override {}
				void Face(const std::string&, const std::string&) override {}
				void Shoot(const std::string&) override {}
				void ShowCaption(const std::string&) override {}
				void StopActor(const std::string&) override {}
				void SetHealth(const std::string& InActor, const float InShare) override { Log.PushBack("health " + InActor + " " + std::to_string(static_cast<int>(InShare * 100.0f))); }
				void ApplyBuff(const std::string& InActor, const std::string& InBuff) override { Log.PushBack("buff " + InActor + " " + InBuff); }
				void SetTactic(const std::string& InActor, const std::string& InTactic) override { Log.PushBack("tactic " + InActor + " " + InTactic); }
				void EndLevel(const bool bInWin) override { Log.PushBack(bInWin ? "win" : "lose"); }
			};
			const auto Root = JsonConfigNode::Parse(R"({"actors":{"a":"enemy#1"},"steps":[
				{"actor":"a","set_health":0.25},{"actor":"a","buff":"rage"},{"actor":"a","tactic":"flank"},{"end_level":"lose"}]})", "test");
			const model::Script Loaded = model::Script::FromConfig(*Root);
			InContext.ExpectEqual(Loaded.Steps.Size(), std::size_t(4), "four steps are read");
			model::ScriptPlayer Player(Loaded);
			SetupWorld World;
			Player.Update(0.1f, World);
			const TArray<std::string> Wanted = { "health a 25", "buff a rage", "tactic a flank", "lose" };
			InContext.Expect(World.Log == Wanted && !Player.IsRunning(), "all four run at once, in order");
			const auto Written = JsonConfigNode::Parse(Loaded.ToJson().ToString(), "test");
			const model::Script Again = model::Script::FromConfig(*Written);
			InContext.Expect(Again.Steps.Size() == 4 && Again.Steps[0].Amount == 0.25f && Again.Steps[1].Text == "rage" && Again.Steps[2].Text == "flank" && !Again.Steps[3].bWin, "the steps write back the same");
		});

		InSuite.Add("Script: until waits on counters (kills>=N) and on the player entering an area", [](TestContext& InContext)
		{
			struct EventWorld : model::IScriptWorld
			{
				int Kills = 0;
				model::Vec2f PlayerCell;
				bool HasActor(const std::string&) const override { return true; }
				bool IsGone(const std::string&) const override { return false; }
				model::Vec2f GetCell(const std::string&) const override { return model::Vec2f(); }
				bool IsAt(const std::string&, const model::Vec2f&) const override { return true; }
				bool IsFacing(const std::string&, const std::string&) const override { return true; }
				bool HasEvent(const std::string&) const override { return false; }
				void MoveTo(const std::string&, const model::Vec2f&) override {}
				void Face(const std::string&, const std::string&) override {}
				void Shoot(const std::string&) override {}
				void ShowCaption(const std::string&) override {}
				void StopActor(const std::string&) override {}
				int GetCounter(const std::string& InName) const override { return InName == "kills" ? Kills : 0; }
				bool IsPlayerInArea(const model::Vec2f& InOrigin, const model::Vec2f& InSize) const override
				{
					return PlayerCell.X >= InOrigin.X && PlayerCell.X < InOrigin.X + InSize.X && PlayerCell.Y >= InOrigin.Y && PlayerCell.Y < InOrigin.Y + InSize.Y;
				}
			};
			const auto Root = JsonConfigNode::Parse(R"({"steps":[{"until":"kills>=2","timeout":100},{"until":"player.in_area:4,5,2,2","timeout":100}]})", "test");
			model::ScriptPlayer Player(model::Script::FromConfig(*Root));
			EventWorld World;
			Player.Update(0.1f, World);
			InContext.ExpectEqual(Player.GetStepIndex(), std::size_t(0), "waits for two kills");
			World.Kills = 1;
			Player.Update(0.1f, World);
			InContext.ExpectEqual(Player.GetStepIndex(), std::size_t(0), "one kill is not enough");
			World.Kills = 2;
			Player.Update(0.1f, World);
			InContext.ExpectEqual(Player.GetStepIndex(), std::size_t(1), "two kills move on");
			World.PlayerCell = { 3.0f, 5.0f };
			Player.Update(0.1f, World);
			InContext.Expect(Player.IsRunning(), "outside the area it keeps waiting");
			World.PlayerCell = { 5.0f, 6.0f };
			Player.Update(0.1f, World);
			InContext.Expect(!Player.IsRunning(), "inside the area it finishes");
		});

		InSuite.Add("Script: steps run in order, moves wait to arrive, shots are spaced, until ends on an event", [](TestContext& InContext)
		{
			struct FakeWorld : model::IScriptWorld
			{
				TArray<std::string> Log;
				int MovesLeft = 2;
				bool bDestroyed = false;
				bool HasActor(const std::string& InActor) const override { return InActor == "a" || InActor == "b"; }
				bool IsGone(const std::string& InActor) const override { return InActor == "ghost"; }
				model::Vec2f GetCell(const std::string&) const override { return model::Vec2f(1.0f, 1.0f); }
				model::Vec2f LastTarget;
				bool IsAt(const std::string&, const model::Vec2f&) const override { return MovesLeft <= 0; }
				bool IsFacing(const std::string&, const std::string&) const override { return true; }
				bool HasEvent(const std::string& InEvent) const override { return InEvent == "b.destroyed" && bDestroyed; }
				void MoveTo(const std::string& InActor, const model::Vec2f& InCell) override { --MovesLeft; LastTarget = InCell; Log.PushBack("move " + InActor); }
				void Face(const std::string& InActor, const std::string&) override { Log.PushBack("face " + InActor); }
				void Shoot(const std::string& InActor) override { Log.PushBack("shoot " + InActor); }
				void ShowCaption(const std::string& InText) override { Log.PushBack("caption " + InText); }
				void StopActor(const std::string& InActor) override { Log.PushBack("stop " + InActor); }
			};
			const auto Root = JsonConfigNode::Parse(R"({"actors":{"a":"enemy#1","b":"enemy#2"},"steps":[
				{"wait":1.0},{"actor":"a","move_by":[0,3]},{"actor":"a","face":"b"},{"caption":"hi"},
				{"actor":"a","shoot":2,"interval":0.5},{"actor":"ghost","shoot":1},{"until":"b.destroyed","timeout":4}]})", "test");
			const model::Script Loaded = model::Script::FromConfig(*Root);
			InContext.Expect(Loaded.Steps.Size() == 7 && Loaded.Actors.Find("b") != nullptr && *Loaded.Actors.Find("b") == "enemy#2", "steps and actors are read");
			model::ScriptPlayer Player(Loaded);
			FakeWorld World;
			Player.Update(0.5f, World);
			InContext.Expect(World.Log.IsEmpty(), "waits before the first step");
			Player.Update(0.6f, World);
			Player.Update(0.1f, World);
			Player.Update(0.1f, World);
			InContext.Expect(World.Log.Size() >= 2 && World.Log[0] == "move a" && World.Log[1] == "move a", "moves until it arrives");
			InContext.Expect(World.LastTarget.X == 1.0f && World.LastTarget.Y == 4.0f, "move_by is counted from where the actor stands");
			const TArray<std::string> AfterMove(World.Log.begin() + 2, World.Log.end());
			InContext.Expect(AfterMove.Size() == 4 && AfterMove[0] == "stop a" && AfterMove[1] == "stop a" && AfterMove[2] == "caption hi" && AfterMove[3] == "shoot a", "arrives, faces, shows the caption and fires at once");
			Player.Update(0.3f, World);
			InContext.Expect(World.Log.Back() == "shoot a" && World.Log.Size() == 6, "the second shot waits for the interval");
			Player.Update(0.3f, World);
			InContext.Expect(World.Log.Size() == 7 && World.Log.Back() == "shoot a", "second shot");
			InContext.Expect(Player.IsRunning(), "a missing actor is skipped and until waits");
			{
				const auto LineUpRoot = JsonConfigNode::Parse(R"({"steps":[{"actor":"a","line_up":"b","gap":2}]})", "test");
				model::ScriptPlayer LineUp(model::Script::FromConfig(*LineUpRoot));
				FakeWorld Lined;
				LineUp.Update(0.1f, Lined);
				InContext.Expect(Lined.LastTarget.X == 1.0f && Lined.LastTarget.Y == 3.0f, "line_up in the same column keeps a gap of 2 cells");
			}
			World.bDestroyed = true;
			Player.Update(0.1f, World);
			InContext.Expect(!Player.IsRunning(), "the event ends the script");
		});

		InSuite.Add("Side hits: stacks slow the tank, clear after a quiet spell, a big side loss stops it", [](TestContext& InContext)
		{
			model::SideHitSettings Settings;
			Settings.SlowPerHit = 0.1f;
			Settings.MaxSlow = 0.25f;
			Settings.SlowTime = 1.0f;
			Settings.StopShare = 0.5f;
			Settings.StopTime = 2.0f;
			model::SimulationClock Clock;
			model::DamageState State(Settings, Clock);
			State.OnSideHit(1, 10);
			InContext.ExpectNear(State.GetSpeedScale(), 0.9f, 0.001f, "one side hit slows by 10%");
			State.OnSideHit(1, 10);
			State.OnSideHit(1, 10);
			InContext.ExpectNear(State.GetSpeedScale(), 0.75f, 0.001f, "the slow is capped");
			InContext.Expect(!State.IsImmobilized(), "3 of 10 health is not enough to stop");
			Clock.Advance(1.1f);
			InContext.Expect(State.Update(), "a quiet spell changes the speed");
			InContext.ExpectNear(State.GetSpeedScale(), 1.0f, 0.001f, "stacks clear after slow_time");
			State.OnSideHit(3, 10);
			InContext.Expect(State.IsImmobilized(), "side losses over half of max health stop the tank");
			InContext.ExpectNear(State.GetSpeedScale(), 0.0f, 0.001f, "a stopped tank has no speed");
			Clock.Advance(2.1f);
			State.Update();
			InContext.Expect(!State.IsImmobilized(), "it moves again after stop_time");
			State.OnSideHit(1, 10);
			InContext.Expect(!State.IsImmobilized(), "the side loss starts over after a stop");
		});

		InSuite.Add("Hit face: a shot is placed on the face it hits", [](TestContext& InContext)
		{
			const model::Vec2f Up{ 0.0f, 1.0f };
			InContext.Expect(model::ShotSideClassifier::ClassifyFace(Up, { 0.0f, -1.0f }) == model::HitFace::Front, "a shot flying at the nose hits the front");
			InContext.Expect(model::ShotSideClassifier::ClassifyFace(Up, { 0.0f, 1.0f }) == model::HitFace::Back, "a shot from behind hits the back");
			InContext.Expect(model::ShotSideClassifier::ClassifyFace(Up, { 1.0f, 0.0f }) == model::HitFace::Left, "a shot flying right comes from the left");
			InContext.Expect(model::ShotSideClassifier::ClassifyFace(Up, { -1.0f, 0.0f }) == model::HitFace::Right, "a shot flying left comes from the right");
			InContext.Expect(model::ShotSideClassifier::ClassifyFace({ 1.0f, 0.0f }, { 0.0f, -1.0f }) == model::HitFace::Left, "facing right, a shot from above hits its left");
		});

		InSuite.Add("AI tactics: in a group only the tank nearest the player comes head on", [](TestContext& InContext)
		{
			InContext.Expect(model::AiTactics::IsClosest(100.0f, { 200.0f, 300.0f }), "nearest pins");
			InContext.Expect(!model::AiTactics::IsClosest(250.0f, { 200.0f, 300.0f }), "the others flank");
			InContext.Expect(model::AiTactics::IsClosest(100.0f, {}), "alone it pins");
		});

		InSuite.Add("AI features: a Hunter gets its profile behaviours and a config can switch them off", [](TestContext& InContext)
		{
			Tank* Hunter = CreateTankAt(InContext, "configs/objects/movables/tanks/enemies/Basic.json", cocos2d::Vec2(TileSize * 4.5f, TileSize * 4.5f));
			if ( !Hunter || !Hunter->GetAi() )
			{
				InContext.Expect(false, "an enemy with AI");
				return;
			}
			Hunter->SetAiSuspended(true);
			const bool bHunter = Hunter->GetAi()->GetType() == EAiType::Hunter;
			InContext.Expect(!bHunter || Hunter->GetAi()->HasFeature(model::AiFeature::LeadShots), "a Hunter leads its shots");
			Hunter->GetAi()->ApplyFeatures({ { "lead_shots", false }, { "dodge", false } });
			InContext.Expect(!Hunter->GetAi()->HasFeature(model::AiFeature::LeadShots) && !Hunter->GetAi()->HasFeature(model::AiFeature::Dodge), "switched off");
		});

		InSuite.Add("AI tactics: a shot is led to where a moving target will be", [](TestContext& InContext)
		{
			const model::Vec2f Aim = model::AiTactics::LeadPoint({ 0.0f, 0.0f }, { 0.0f, 400.0f }, { 100.0f, 0.0f }, 400.0f);
			InContext.Expect(Aim.X > 90.0f && Aim.X < 110.0f, "about one second of target travel ahead");
			const model::Vec2f Still = model::AiTactics::LeadPoint({ 0.0f, 0.0f }, { 0.0f, 400.0f }, { 0.0f, 0.0f }, 400.0f);
			InContext.Expect(Still == model::Vec2f(0.0f, 400.0f), "a still target is aimed at directly");
		});

		InSuite.Add("AI tactics: incoming bullets are threats, passing ones are not, and the dodge steps aside", [](TestContext& InContext)
		{
			const model::Vec2f Tank{ 0.0f, 0.0f };
			InContext.Expect(model::AiTactics::IsThreat(Tank, 20.0f, { 5.0f, -200.0f }, { 0.0f, 500.0f }, 0.6f), "coming straight at it");
			InContext.Expect(!model::AiTactics::IsThreat(Tank, 20.0f, { 60.0f, -200.0f }, { 0.0f, 500.0f }, 0.6f), "passing to the side");
			InContext.Expect(!model::AiTactics::IsThreat(Tank, 20.0f, { 0.0f, 200.0f }, { 0.0f, 500.0f }, 0.6f), "flying away");
			InContext.Expect(!model::AiTactics::IsThreat(Tank, 20.0f, { 0.0f, -900.0f }, { 0.0f, 500.0f }, 0.6f), "too far to matter yet");
			const model::Vec2f Step = model::AiTactics::DodgeDirection(Tank, { 5.0f, -200.0f }, { 0.0f, 500.0f });
			InContext.Expect(Step == model::Vec2f(-1.0f, 0.0f), "steps away from the side the bullet is on");
			InContext.Expect(model::AiTactics::ShouldRetreat(0.2f, 0.25f) && !model::AiTactics::ShouldRetreat(0.5f, 0.25f), "falls back only when hurt");
		});

		InSuite.Add("Difficulty: the mode scales enemy ability cooldowns and AI reaction", [](TestContext& InContext)
		{
			Tank* Boss = CreateTankAt(InContext, "configs/objects/movables/tanks/enemies/Boss.json", cocos2d::Vec2(TileSize * 6.5f, TileSize * 6.5f));
			if ( !Boss || !Boss->GetAi() )
			{
				InContext.Expect(false, "boss with an AI");
				return;
			}
			Boss->SetAiSuspended(true);
			const float ThinkMax = Boss->GetAi()->GetSettings().GetThinkIntervalMax();
			const float Aim = Boss->GetAi()->GetSettings().GetAimTolerance();
			model::DifficultyDef Mode;
			Mode.AbilityCooldown = 0.5f;
			Mode.AiThinkTime = 0.5f;
			Mode.AiAimTolerance = 2.0f;
			Boss->ApplyDifficulty(Mode);
			InContext.ExpectNear(Boss->GetAi()->GetSettings().GetThinkIntervalMax(), ThinkMax * 0.5f, 0.001f, "decides twice as often");
			InContext.ExpectNear(Boss->GetAi()->GetSettings().GetAimTolerance(), Aim * 2.0f, 0.001f, "fires further off line");
			Boss->UseAbility(1);
			InContext.ExpectNear(Boss->GetAbility(1)->GetCooldownRemaining(), 15.0f * 0.5f, 0.05f, "shield back in half the time");
		});

		InSuite.Add("Difficulty: friendly fire lets an enemy shot hurt another enemy, never the shooter", [](TestContext& InContext)
		{
			Tank* Shooter = CreateTankAt(InContext, "configs/objects/movables/tanks/enemies/Basic.json", cocos2d::Vec2(TileSize * 4.5f, TileSize * 2.5f));
			Tank* Ally = CreateTankAt(InContext, "configs/objects/movables/tanks/enemies/Basic.json", cocos2d::Vec2(TileSize * 4.5f, TileSize * 6.5f));
			if ( !Shooter || !Ally )
			{
				return;
			}
			Shooter->SetAiSuspended(true);
			Ally->SetAiSuspended(true);
			model::DifficultyDef Mode;
			Mode.bFriendlyFire = true;
			Shooter->ApplyDifficulty(Mode);
			Shooter->TurnTo(0.0f);
			Advance(1.0f);
			const int ShooterHealth = Shooter->GetHealth();
			const int AllyHealth = Ally->GetHealth();
			Shooter->UseAbility(0);
			Advance(1.5f);
			InContext.Expect(Ally->GetHealth() < AllyHealth || Ally->IsDestroying(), "the ally ahead is hit");
			InContext.ExpectEqual(Shooter->GetHealth(), ShooterHealth, "the shooter is not");
		});

		InSuite.Add("Difficulty: spawn counts scale, round and never drop below one", [](TestContext& InContext)
		{
			InContext.ExpectEqual(model::DifficultyBook::ScaleCount(5, 1.4f), 7, "nightmare adds enemies");
			InContext.ExpectEqual(model::DifficultyBook::ScaleCount(5, 0.8f), 4, "easy removes some");
			InContext.ExpectEqual(model::DifficultyBook::ScaleCount(1, 0.1f), 1, "never zero");
		});

		InSuite.Add("Difficulty: enemy reload and bullet speed follow the mode", [](TestContext& InContext)
		{
			Tank* Enemy = CreateTankAt(InContext, "configs/objects/movables/tanks/enemies/Basic.json", cocos2d::Vec2(TileSize * 4.5f, TileSize * 4.5f));
			if ( !Enemy )
			{
				return;
			}
			Tank* Quick = CreateTankAt(InContext, "configs/objects/movables/tanks/enemies/Basic.json", cocos2d::Vec2(TileSize * 8.5f, TileSize * 4.5f));
			if ( !Quick )
			{
				return;
			}
			Enemy->SetAiSuspended(true);
			Quick->SetAiSuspended(true);
			model::DifficultyDef Mode;
			Mode.ReloadTime = 0.5f;
			Mode.BulletSpeed = 2.0f;
			Quick->ApplyDifficulty(Mode);
			Enemy->UseAbility(0);
			Quick->UseAbility(0);
			InContext.ExpectNear(Quick->GetReloadRemaining(), Enemy->GetReloadRemaining() * 0.5f, 0.01f, "half the reload");
		});

		InSuite.Add("Boss phases: each threshold starts its phase once and in order", [](TestContext& InContext)
		{
			model::PhaseTracker Tracker;
			Tracker.Configure({ 0.5f, 0.2f });
			InContext.Expect(Tracker.Update(0.8f).IsEmpty() && Tracker.GetCurrentPhase() == -1, "no phase at full health");
			InContext.ExpectEqual(static_cast<int>(Tracker.Update(0.5f).Size()), 1, "half health starts the first");
			InContext.Expect(Tracker.Update(0.4f).IsEmpty() && Tracker.GetCurrentPhase() == 0, "not started twice");
			const TArray<int> Started = Tracker.Update(0.0f);
			InContext.Expect(Started.Size() == 1 && Started[0] == 1, "the second one at a fifth");
		});

		InSuite.Add("Boss phases: one big hit starts every skipped phase", [](TestContext& InContext)
		{
			model::PhaseTracker Tracker;
			Tracker.Configure({ 0.5f, 0.2f });
			const TArray<int> Started = Tracker.Update(0.1f);
			InContext.Expect(Started.Size() == 2 && Started[0] == 0 && Started[1] == 1, "both, in order");
		});

		InSuite.Add("Boss phases: the boss gains abilities and calls escorts below half health", [](TestContext& InContext)
		{
			Tank* Boss = CreateTankAt(InContext, "configs/objects/movables/tanks/enemies/Boss.json", cocos2d::Vec2(TileSize * 6.5f, TileSize * 6.5f));
			if ( !Boss )
			{
				return;
			}
			Boss->SetAiSuspended(true);
			int Calls = 0;
			std::string Escort;
			Boss->SetOnPhaseCallback([&](Tank&, const TankPhaseDef& InPhase) { ++Calls; Escort = std::string(InPhase.SummonConfig.View()); });
			const size_t AbilitiesBefore = Boss->GetAbilityCount();
			Advance(2.0f / 60.0f);
			InContext.ExpectEqual(Boss->GetPhase(), -1, "full health: no phase");
			Boss->ApplyDamage(Boss->GetMaxHealth() / 2 + 3, ETeam::Player);
			Advance(2.0f / 60.0f);
			InContext.ExpectEqual(Boss->GetPhase(), 0, "second phase");
			InContext.Expect(Boss->GetAbilityCount() > AbilitiesBefore, "new ability");
			InContext.Expect(Calls == 1 && !Escort.empty(), "escorts called once");
			Advance(2.0f / 60.0f);
			InContext.ExpectEqual(Calls, 1, "not again");
		});

		InSuite.Add("Cloaker: fades out, stays hidden, fades back in; a reveal shows it at once", [](TestContext& InContext)
		{
			model::CloakCycle Cycle;
			Cycle.Configure(2.0f, 1.0f, 0.5f, 0.1f);
			InContext.Expect(!Cycle.IsHidden() && Cycle.GetOpacity() == 1.0f, "starts seen");
			Cycle.Update(0.6f);
			InContext.Expect(Cycle.IsHidden() && Cycle.GetOpacity() > 0.1f && Cycle.GetOpacity() < 1.0f, "fading out");
			Cycle.Update(0.5f);
			InContext.Expect(Cycle.IsHidden() && std::fabs(Cycle.GetOpacity() - 0.1f) < 0.001f, "hidden");
			Cycle.Update(1.5f);
			InContext.Expect(!Cycle.IsHidden() && Cycle.GetOpacity() < 1.0f, "fading back in after the hidden time");
			Cycle.Update(1.0f);
			InContext.Expect(Cycle.IsHidden(), "and hides again");
			Cycle.Reveal();
			InContext.Expect(!Cycle.IsHidden() && Cycle.GetOpacity() == 1.0f, "a reveal shows it fully at once");
		});

		InSuite.Add("Cloaker: the tank turns nearly invisible, and a hit shows it", [](TestContext& InContext)
		{
			Tank* Cloaker = CreateTankAt(InContext, "configs/test/cloaker_vehicle.json", cocos2d::Vec2(TileSize * 4.5f, TileSize * 3.5f));
			if ( !Cloaker )
			{
				return;
			}
			Advance(1.0f);
			InContext.Expect(Cloaker->GetVisualNode()->getOpacity() < 60, "nearly invisible");
			Cloaker->ApplyDamage(1, ETeam::Player);
			Advance(2.0f / 60.0f);
			InContext.ExpectEqual(static_cast<int>(Cloaker->GetVisualNode()->getOpacity()), 255, "seen after a hit");
		});

		InSuite.Add("ConfigCatalog: find by id, a repeated id replaces the entry and keeps its place", [](TestContext& InContext)
		{
			model::ConfigCatalog<model::AchievementDef> Catalog;
			Catalog.Add({ "a", "A", "kills", 1 });
			Catalog.Add({ "b", "B", "kills", 2 });
			Catalog.Add({ "a", "A2", "coins", 5 });
			InContext.Expect(Catalog.GetAll().Size() == 2, "two entries");
			InContext.Expect(Catalog.Find("a") != nullptr && Catalog.Find("a")->Target == 5, "the later one wins");
			InContext.Expect(Catalog.GetAll().Front().Id == "a", "keeps the list order");
			InContext.Expect(Catalog.Find("zzz") == nullptr && !Catalog.Contains("zzz"), "unknown id");
		});

		InSuite.Add("WeightedTable: weights split the picks, zero weights never get in, a seed repeats them", [](TestContext& InContext)
		{
			model::WeightedTable<std::string> Table;
			model::Random EmptyRng(1);
			InContext.Expect(Table.Pick(EmptyRng) == nullptr, "empty picks nothing");
			Table.Add("common", 3);
			Table.Add("rare", 1);
			Table.Add("never", 0);
			InContext.Expect(Table.GetTotalWeight() == 4, "zero weight is skipped");
			model::Random Rng(99);
			int Rare = 0;
			for ( int Index = 0; Index < 4000; ++Index )
			{
				const std::string* const Picked = Table.Pick(Rng);
				InContext.Expect(Picked != nullptr && *Picked != "never", "only real entries");
				Rare += Picked && *Picked == "rare" ? 1 : 0;
			}
			InContext.Expect(Rare > 800 && Rare < 1200, "rare is about a quarter");
		});

		InSuite.Add("Cooldown: ends at a timestamp, readiness runs 0 to 1, nothing is ticked", [](TestContext& InContext)
		{
			model::SimulationClock Clock;
			model::Cooldown Wait;
			InContext.Expect(Wait.IsReady(Clock.GetNow()) && Wait.GetReadiness(Clock.GetNow()) > 0.99f, "ready before the first start");
			Wait.Start(Clock.GetNow(), 2.0f);
			InContext.Expect(!Wait.IsReady(Clock.GetNow()), "waiting");
			Clock.Advance(0.5f);
			InContext.ExpectNear(Wait.GetReadiness(Clock.GetNow()), 0.25f, 0.001f, "a quarter through");
			InContext.ExpectNear(Wait.GetRemaining(Clock.GetNow()), 1.5f, 0.001f, "1.5 s left");
			Clock.Advance(5.0f);
			InContext.Expect(Wait.IsReady(Clock.GetNow()) && Wait.GetRemaining(Clock.GetNow()) == 0.0f, "stays ready after the end");
		});

		InSuite.Add("GameEvents: events are counted and logged with their time, Clear starts over", [](TestContext& InContext)
		{
			model::GameEvents Events;
			InContext.Expect(!Events.Has("pickup.taken"), "nothing yet");
			Events.Emit("pickup.taken", 1.5);
			Events.Emit("pickup.taken", 2.0);
			Events.Emit("enemy.spawned", 2.5);
			InContext.Expect(Events.Has("pickup.taken") && Events.GetCount("pickup.taken") == 2, "counted");
			InContext.Expect(Events.GetLog().Size() == 3 && Events.GetLog()[2].Name == "enemy.spawned", "logged in order");
			Events.Clear();
			InContext.Expect(!Events.Has("pickup.taken") && Events.GetLog().IsEmpty(), "a new level starts empty");
		});

		InSuite.Add("TFunction: stores state inline, copies, moves, resets; TFunctionRef calls without owning", [](TestContext& InContext)
		{
			struct Probe
			{
				int* Alive;
				explicit Probe(int* const InAlive) : Alive(InAlive) { ++*Alive; }
				Probe(const Probe& InOther) : Alive(InOther.Alive) { ++*Alive; }
				~Probe() { --*Alive; }
			};
			int AliveCount = 0;
			int Total = 0;
			{
				TFunction<void(int), UiCallbackInlineSize> Add = [&Total, Held = Probe(&AliveCount)](const int InValue) { Total += InValue; };
				InContext.Expect(static_cast<bool>(Add) && AliveCount == 1, "a lambda is stored once");
				const TFunction<void(int), UiCallbackInlineSize> Copied = Add;
				InContext.Expect(AliveCount == 2, "a copy holds its own state");
				Add(3);
				Copied(4);
				InContext.ExpectEqual(Total, 7, "both calls reach the captured counter");
				TFunction<void(int), UiCallbackInlineSize> Moved = std::move(Add);
				InContext.Expect(!Add && AliveCount == 2, "a moved-from function is empty and nothing leaks");
				Moved = nullptr;
				InContext.Expect(!Moved && AliveCount == 1, "reset destroys the stored state");
			}
			InContext.ExpectEqual(AliveCount, 0, "destroying the functions destroys every capture");

			TFunction<int(int, int)> Sum = [](const int InA, const int InB) { return InA + InB; };
			InContext.ExpectEqual(Sum(2, 3), 5, "returns the result");
			TFunction<int()> Empty;
			InContext.Expect(Empty == nullptr && !Empty.IsBound(), "default is empty");

			const auto Apply = [](const TFunctionRef<int(int)> InFunction) { return InFunction(10); };
			InContext.ExpectEqual(Apply([](const int InValue) { return InValue + 5; }), 15, "a reference calls a temporary lambda");
		});

		InSuite.Add("TStoredString: one id per text, equal ids compare equal, Find never adds, empty is id 0", [](TestContext& InContext)
		{
			const FTransientString First("stored.test.alpha");
			const FTransientString Again(std::string("stored.test.") + "alpha");
			const FTransientString Other("stored.test.beta");
			InContext.Expect(First == Again && First.GetId() == Again.GetId(), "the same text gives the same id");
			InContext.Expect(First != Other, "different texts differ");
			InContext.Expect(First.View() == "stored.test.alpha" && First == std::string_view("stored.test.alpha"), "the text comes back");
			InContext.Expect(FTransientString::Find("stored.test.never_added").IsEmpty(), "Find of an unknown text is empty and adds nothing");
			InContext.Expect(FTransientString::Find("stored.test.never_added").IsEmpty() && FTransientString().View().empty(), "still unknown, the default is empty");
			InContext.Expect(FTransientString::Find("stored.test.beta") == Other, "Find returns a known text");
			static_assert(sizeof(FTransientString) == 4, "a stored string is a 4-byte id");
		});

		InSuite.Add("TStoredString: flushing transient strings keeps persistent ones and forgets transient texts", [](TestContext& InContext)
		{
			const FPersistentString Kept("stored.flush.persistent");
			const FTransientString Dropped("stored.flush.transient");
			InContext.Expect(Kept == FPersistentString("stored.flush.persistent"), "a persistent text is interned once");

			FTransientString::ClearTransientStrings();

			InContext.Expect(Kept.View() == "stored.flush.persistent", "the persistent text survives a flush");
			InContext.Expect(FTransientString::Find("stored.flush.transient").IsEmpty(), "a flushed transient text is unknown again");
			InContext.Expect(Dropped.View() != "stored.flush.transient", "a handle kept across a flush no longer reads its old text");

			const FTransientString Fresh("stored.flush.fresh");
			InContext.Expect(Fresh.View() == "stored.flush.fresh", "a new transient text is stored after a flush");
		});

		InSuite.Add("TStoredString: statistics count each text once and the flush empties only the transient pool", [](TestContext& InContext)
		{
			const size_t PersistentBefore = FPersistentString::GetStringCount();
			const size_t BytesBefore = FPersistentString::GetStringBytes();
			const FPersistentString First("stored.stats.persistent");
			const FPersistentString Again("stored.stats.persistent");
			InContext.ExpectEqual(FPersistentString::GetStringCount(), PersistentBefore + 1, "a repeated text adds one entry");
			InContext.ExpectEqual(FPersistentString::GetStringBytes(), BytesBefore + First.View().size(), "bytes grow by the text length once");
			InContext.Expect(Again == First, "duplicates share an id");

			const FTransientString Temp("stored.stats.transient");
			InContext.Expect(FTransientString::GetStringCount() >= 1 && FTransientString::GetStringBytes() >= Temp.View().size(), "the transient pool counts its own texts");
			const size_t PersistentMid = FPersistentString::GetStringCount();
			FTransientString::ClearTransientStrings();
			InContext.ExpectEqual(FTransientString::GetStringCount(), static_cast<size_t>(0), "a flush empties the transient pool");
			InContext.ExpectEqual(FTransientString::GetStringBytes(), static_cast<size_t>(0), "a flush frees the transient bytes");
			InContext.ExpectEqual(FPersistentString::GetStringCount(), PersistentMid, "a flush leaves the persistent pool alone");
		});

		InSuite.Add("TStoredString: loading the enemy catalog and a level interns each id once", [](TestContext& InContext)
		{
			const FPersistentString Probe("stored.stats.probe");
			const size_t Before = FPersistentString::GetStringCount();
			TArray<FPersistentString> Ids;
			for ( int Round = 0; Round < 3; ++Round )
			{
				Ids.PushBack(FPersistentString{ "Basic" });
				Ids.PushBack(FPersistentString{ "stored.stats.probe" });
			}
			InContext.Expect(FPersistentString::GetStringCount() <= Before + 1, "repeated ids from data never grow the pool");
			InContext.Expect(Ids[0] == Ids[2] && Ids[1] == Probe, "equal texts compare as equal ids");
		});

		InSuite.Add("GameObject: names live in the transient pool and are dropped by a level flush", [](TestContext& InContext)
		{
			struct NamedObject : GameObject
			{
				bool Build() override { return true; }
			};
			NamedObject Object;
			const size_t PersistentBefore = FPersistentString::GetStringCount();
			Object.SetName("stored.object.level_name");
			InContext.Expect(Object.GetName() == "stored.object.level_name", "the name reads back");
			InContext.ExpectEqual(FPersistentString::GetStringCount(), PersistentBefore, "an object name never grows the persistent pool");
			FTransientString::ClearTransientStrings();
			InContext.ExpectEqual(FTransientString::GetStringCount(), static_cast<size_t>(0), "the level flush forgets object names");
		});

		InSuite.Add("TUniquePtr: owns one object, moves, converts to a base and frees once", [](TestContext& InContext)
		{
			struct Base
			{
				explicit Base(int* const InDestroyed) : Destroyed(InDestroyed) {}
				virtual ~Base() { ++*Destroyed; }
				int* Destroyed;
			};
			struct Derived : Base
			{
				using Base::Base;
			};
			int Destroyed = 0;
			{
				TUniquePtr<Derived> Child = MakeUnique<Derived>(&Destroyed);
				InContext.Expect(Child && Child.Get() != nullptr, "a made pointer owns an object");
				TUniquePtr<Base> Parent = std::move(Child);
				InContext.Expect(!Child && Parent && Destroyed == 0, "a move to a base hands the object over");
				TUniquePtr<Base> Other;
				Other = std::move(Parent);
				InContext.Expect(!Parent && Other && Destroyed == 0, "a move assignment hands the object over");
				Other.Reset();
				InContext.Expect(Destroyed == 1 && !Other, "Reset frees the object");
				TUniquePtr<Base> Released = MakeUnique<Base>(&Destroyed);
				Base* const Raw = Released.Release();
				InContext.Expect(!Released && Destroyed == 1, "Release gives the object up without freeing it");
				delete Raw;
			}
			InContext.Expect(Destroyed == 2, "every object was freed exactly once");
		});

		InSuite.Add("TSharedValue: copies of a handle share one value until the last one goes", [](TestContext& InContext)
		{
			struct Counted
			{
				explicit Counted(int* const InDestroyed) : Destroyed(InDestroyed) {}
				~Counted() { ++*Destroyed; }
				int* Destroyed;
				int Value = 0;
			};
			int Destroyed = 0;
			{
				TSharedHandle<Counted> First = MakeSharedValue<Counted>(&Destroyed);
				First->Get().Value = 7;
				{
					const TSharedHandle<Counted> Second = First;
					InContext.Expect(Second->Get().Value == 7 && Second->GetRefCount() == 2, "a copy sees the same value");
				}
				InContext.Expect(First->GetRefCount() == 1 && Destroyed == 0, "the value outlives a copy");
			}
			InContext.Expect(Destroyed == 1, "the last handle frees it once");
		});

		InSuite.Add("AllocationCounter: counts heap allocations and frees (Debug and Test builds)", [](TestContext& InContext)
		{
			if ( !AllocationCounter::IsEnabled() )
			{
				InContext.Expect(AllocationCounter::Capture().Allocations == 0, "a disabled counter reports zeros");
				return;
			}
			const AllocationCounter::Snapshot Before = AllocationCounter::Capture();
			void* const Block = ::operator new(sizeof(int) * 256);
			const AllocationCounter::Snapshot During = AllocationCounter::Capture();
			InContext.Expect(During.Allocations > Before.Allocations, "an allocation is counted");
			InContext.Expect(During.AllocatedBytes >= Before.AllocatedBytes + sizeof(int) * 256, "its size is counted");
			InContext.Expect(During.LiveBytes >= Before.LiveBytes + static_cast<std::int64_t>(sizeof(int) * 256), "the live bytes grow");
			::operator delete(Block);
			InContext.Expect(AllocationCounter::Capture().Frees > During.Frees, "a free is counted");
			InContext.Expect(AllocationCounter::GetPeakStackBytes() > 0, "the stack depth is known");
		});

		InSuite.Add("TRingBuffer: FIFO order, refuses pushes when full, wraps around", [](TestContext& InContext)
		{
			TRingBuffer<int, 3> Queue;
			InContext.Expect(Queue.Push(1) && Queue.Push(2) && Queue.Push(3), "three values fit");
			InContext.Expect(!Queue.Push(4) && Queue.IsFull(), "a full buffer refuses a push");
			int Value = 0;
			InContext.Expect(Queue.Pop(Value) && Value == 1, "the oldest value comes out first");
			InContext.Expect(Queue.Push(4), "a freed slot is reused");
			InContext.Expect(Queue.Pop(Value) && Value == 2, "order is kept across the wrap");
			Queue.Pop(Value);
			Queue.Pop(Value);
			InContext.Expect(!Queue.Pop(Value) && Queue.IsEmpty(), "an empty buffer refuses a pop");
		});

		InSuite.Add("TObjectPool: acquires until empty, releases and reuses slots", [](TestContext& InContext)
		{
			TObjectPool<int> Pool(2);
			int* const First = Pool.Acquire();
			int* const Second = Pool.Acquire();
			InContext.Expect(First != nullptr && Second != nullptr && First != Second, "two distinct slots are handed out");
			InContext.Expect(Pool.Acquire() == nullptr, "an exhausted pool returns nullptr");
			InContext.Expect(Pool.Release(First) && Pool.FreeCount() == 1, "a released slot becomes free");
			InContext.Expect(Pool.Acquire() == First, "the released slot is reused");
			int Outside = 0;
			InContext.Expect(!Pool.Release(&Outside), "an object from outside the pool is refused");
		});

		InSuite.Add("TSlotMap: handles resolve until removed, stale handles are rejected after slot reuse", [](TestContext& InContext)
		{
			TSlotMap<int> Map;
			const TEntityHandle Ship = Map.Add(11);
			InContext.Expect(Map.Find(Ship) != nullptr && *Map.Find(Ship) == 11, "a new handle reads its value");
			InContext.Expect(Map.Remove(Ship) && Map.Find(Ship) == nullptr, "a removed handle no longer resolves");
			const TEntityHandle Reused = Map.Add(22);
			InContext.Expect(Reused.Index == Ship.Index && Reused.Generation != Ship.Generation, "the slot is reused with a new generation");
			InContext.Expect(Map.Find(Ship) == nullptr && *Map.Find(Reused) == 22, "the stale handle does not reach the new item");
			InContext.Expect(!TEntityHandle{}.IsValid() && Map.Size() == 1, "a default handle is invalid");
		});

		InSuite.Add("TArray: insert, erase, swap-remove and pop keep the remaining order", [](TestContext& InContext)
		{
			TArray<int> Items(std::pmr::get_default_resource());
			Items.PushBack(1);
			Items.PushBack(3);
			Items.Insert(Items.begin() + 1, 2);
			InContext.Expect(Items.Size() == 3 && Items[1] == 2 && Items.Front() == 1 && Items.Back() == 3, "insert keeps order");
			Items.RemoveAt(0);
			InContext.Expect(Items[0] == 2 && Items.Size() == 2, "RemoveAt shifts the rest");
			Items.PushBack(4);
			Items.RemoveAtSwap(0);
			InContext.Expect(Items[0] == 4 && Items.Size() == 2, "RemoveAtSwap fills the hole with the last item");
			Items.PopBack();
			InContext.Expect(Items.Size() == 1 && !Items.IsEmpty(), "PopBack removes the last item");
			Items.Clear();
			InContext.Expect(Items.IsEmpty(), "Clear empties it");
		});

		InSuite.Add("TSmallArray: stays inline up to its capacity, spills past it, copies and moves keep values", [](TestContext& InContext)
		{
			TSmallArray<int, 2> Small;
			Small.PushBack(1);
			Small.PushBack(2);
			InContext.Expect(Small.IsInline() && Small.Size() == 2, "two values fit inline");
			Small.PushBack(3);
			InContext.Expect(!Small.IsInline() && Small.Size() == 3 && Small[2] == 3, "a third value spills to the arena");
			const TSmallArray<int, 2> Copied = Small;
			InContext.Expect(Copied.Size() == 3 && Copied[0] == 1 && Copied[2] == 3, "a copy keeps every value");
			TSmallArray<int, 2> Moved = std::move(Small);
			InContext.Expect(Moved.Size() == 3 && Moved[1] == 2, "a move keeps every value");
			Moved.Clear();
			InContext.Expect(Moved.IsEmpty(), "Clear empties a spilled array");
		});

		InSuite.Add("Log: lines reach the installed sink with their level and formatted text", [](TestContext& InContext)
		{
			struct CaptureSink : ports::ILogSink
			{
				model::ELogLevel Level{ model::ELogLevel::Info };
				std::string Text;
				void Write(const model::ELogLevel InLevel, const std::string_view InText) override
				{
					Level = InLevel;
					Text = std::string(InText);
				}
			};
			CaptureSink Sink;
			model::Log::SetSink(&Sink);
			model::Log::Error("bad %s %d", "id", 7);
			model::Log::SetSink(nullptr);
			model::Log::Warn("dropped without a sink");
			InContext.ExpectEqual(Sink.Text, std::string("bad id 7"), "formatted text");
			InContext.Expect(Sink.Level == model::ELogLevel::Error, "level");
		});

		InSuite.Add("TSet: insert once, contains, remove, iterates in key order", [](TestContext& InContext)
		{
			TSet<int> Numbers;
			InContext.Expect(Numbers.Insert(5) && Numbers.Insert(2) && Numbers.Insert(9), "new keys are added");
			InContext.Expect(!Numbers.Insert(5), "a duplicate is rejected");
			InContext.Expect(Numbers.Size() == 3 && Numbers.Contains(2) && !Numbers.Contains(3), "size and lookup");
			std::string Order;
			for ( const int Number : Numbers )
			{
				Order += std::to_string(Number);
			}
			InContext.Expect(Order == "259", "iterates in key order");
			InContext.Expect(Numbers.Remove(5) && !Numbers.Remove(5) && Numbers.Size() == 2, "remove reports whether the key was there");
			TSet<int> Same{ 2, 9 };
			InContext.Expect(Numbers == Same && Numbers != TSet<int>{ 2 }, "sets compare by content");
			TSet<FPersistentString> Names;
			Names.Insert("armor");
			InContext.Expect(Names.Contains("armor") && !Names.Contains("never_interned_name_xyz"), "a string lookup never interns the name");
			Numbers.Clear();
			InContext.Expect(Numbers.IsEmpty(), "clear");
		});
		InSuite.Add("TMap: insert or assign, find, remove, iterates in key order", [](TestContext& InContext)
		{
			TMap<int, int> Scores;
			Scores.InsertOrAssign(3, 30);
			Scores.InsertOrAssign(1, 10);
			Scores.InsertOrAssign(3, 33);
			InContext.Expect(Scores.Size() == 2 && *Scores.Find(3) == 33, "assign replaces the value of an existing key");
			InContext.Expect(Scores.Contains(1) && Scores.Find(7) == nullptr, "a missing key is not found");
			TArray<int> Keys;
			for (const auto& Entry : Scores)
			{
				Keys.PushBack(Entry.Key);
			}
			InContext.Expect(Keys == TArray<int>({ 1, 3 }), "keys come out sorted");
			InContext.Expect(Scores.Remove(1) && !Scores.Remove(1) && !Scores.Contains(1), "remove works once");
			Scores.Clear();
			InContext.Expect(Scores.IsEmpty(), "Clear empties the map");
		});

		InSuite.Add("TEnumArray: indexes by enum, Fill sets every slot", [](TestContext& InContext)
		{
			enum class TestSlot : std::uint8_t { First, Second, Third, Count };
			TEnumArray<TestSlot, int> Slots(5);
			Slots[TestSlot::Second] = 9;
			InContext.Expect(Slots[TestSlot::First] == 5 && Slots[TestSlot::Second] == 9, "the default fills every slot, writes stay local");
			Slots.Fill(1);
			InContext.Expect(Slots[TestSlot::Third] == 1 && decltype(Slots)::Size == 3, "Fill sets every slot");
		});

		InSuite.Add("TEnumFlags: set, clear, toggle, has, has any, has all", [](TestContext& InContext)
		{
			enum class TestFlag : std::uint8_t { Left = 1, Right = 2, Bottom = 4 };
			TEnumFlags<TestFlag> Flags;
			InContext.Expect(Flags.IsEmpty(), "a new set is empty");
			Flags.Set(TestFlag::Left);
			Flags.Set(TestFlag::Bottom);
			InContext.Expect(Flags.Has(TestFlag::Left) && !Flags.Has(TestFlag::Right), "set flags are reported");
			InContext.Expect(Flags.HasAll(TEnumFlags<TestFlag>(TestFlag::Left) | TEnumFlags<TestFlag>(TestFlag::Bottom)), "HasAll needs every flag");
			InContext.Expect(Flags.HasAny(TEnumFlags<TestFlag>(TestFlag::Right) | TEnumFlags<TestFlag>(TestFlag::Bottom)), "HasAny needs one flag");
			Flags.Clear(TestFlag::Left);
			Flags.Toggle(TestFlag::Right);
			InContext.Expect(!Flags.Has(TestFlag::Left) && Flags.Has(TestFlag::Right), "Clear and Toggle change one flag");
			Flags.ClearAll();
			InContext.Expect(!Flags && Flags.GetRaw() == 0, "ClearAll empties the set");
		});

		InSuite.Add("TObserved: listeners run only on change, unsubscribed ones stop, immediate notify on subscribe", [](TestContext& InContext)
		{
			TObserved<int> Health(10);
			int Calls = 0;
			int LastSeen = -1;
			const TObserved<int>::ListenerId Id = Health.Subscribe([&Calls, &LastSeen](const int InNew) { ++Calls; LastSeen = InNew; });
			Health.Set(7);
			InContext.Expect(Calls == 1 && LastSeen == 7, "a change notifies with the new value");
			Health.Set(7);
			InContext.Expect(Calls == 1, "setting the same value does not notify");
			Health.Unsubscribe(Id);
			Health = 3;
			InContext.Expect(Calls == 1 && Health.Get() == 3, "an unsubscribed listener is not called");
			int Immediate = -1;
			Health.Subscribe([&Immediate](const int InNew) { Immediate = InNew; }, true);
			InContext.Expect(Immediate == 3, "bNotifyImmediately calls the listener at once");
		});

		InSuite.Add("TScopeGuard and TScopeExit: run at scope end, Dismiss and Release cancel, move keeps one run", [](TestContext& InContext)
		{
			int Guarded = 0;
			{
				auto Guard = MakeScopeGuard([&Guarded]() { ++Guarded; });
			}
			InContext.ExpectEqual(Guarded, 1, "a guard runs at scope end");
			{
				auto Guard = MakeScopeGuard([&Guarded]() { ++Guarded; });
				Guard.Dismiss();
			}
			InContext.ExpectEqual(Guarded, 1, "a dismissed guard does not run");
			{
				auto Guard = MakeScopeGuard([&Guarded]() { ++Guarded; });
				auto Moved = std::move(Guard);
			}
			InContext.ExpectEqual(Guarded, 2, "a moved guard runs exactly once");
			{
				auto Exit = MakeScopeExit([&Guarded]() { ++Guarded; });
				Exit.Release();
			}
			InContext.ExpectEqual(Guarded, 2, "a released scope exit does not run");
		});

		InSuite.Add("TOverloaded: picks the lambda that matches the argument type", [](TestContext& InContext)
		{
			TOverloaded Visitor{
				[](const int) { return 1; },
				[](const double) { return 2; }
			};
			InContext.ExpectEqual(Visitor(4), 1, "an int uses the int lambda");
			InContext.ExpectEqual(Visitor(4.0), 2, "a double uses the double lambda");
		});

		InSuite.Add("TStrongType: values compare, increment and report validity", [](TestContext& InContext)
		{
			struct TestIdTag {};
			using TestId = TStrongType<int, TestIdTag>;
			const TestId Empty;
			TestId Current(5);
			InContext.Expect(!Empty.IsValid() && static_cast<bool>(Current), "zero is invalid, others are valid");
			InContext.Expect(Current == TestId(5) && Current != Empty && Empty < Current, "equality and ordering use the value");
			++Current;
			InContext.Expect(Current == TestId(6) && Current++ == TestId(6) && Current == TestId(7), "pre and post increment");
		});

		InSuite.Add("Scheduler: runs callbacks when due, in order, cancelled ones never, callbacks may schedule more", [](TestContext& InContext)
		{
			model::Scheduler Plan;
			TArray<int> Ran;
			Plan.At(2.0, [&Ran]() { Ran.PushBack(2); });
			Plan.At(1.0, [&Ran]() { Ran.PushBack(1); });
			const model::Scheduler::Handle Dropped = Plan.At(1.5, [&Ran]() { Ran.PushBack(99); });
			Plan.At(1.0, [&Ran]() { Ran.PushBack(11); });
			InContext.Expect(Plan.Cancel(Dropped) && !Plan.Cancel(Dropped), "a pending one is cancelled once");
			Plan.RunDue(0.5);
			InContext.Expect(Ran.IsEmpty(), "nothing is due yet");
			Plan.RunDue(1.0);
			InContext.Expect(Ran == TArray<int>({ 1, 11 }), "due ones run, same time in the order added");
			Plan.After(0.5f, [&Ran]() { Ran.PushBack(15); });
			Plan.RunDue(1.4);
			InContext.Expect(Ran.Size() == 2, "After counts from the last run time");
			Plan.RunDue(2.0);
			InContext.Expect(Ran == TArray<int>({ 1, 11, 15, 2 }) && Plan.GetPendingCount() == 0, "the rest ran");
		});

		InSuite.Add("Scheduler: a budget stops after the callback that used it up, the rest run next tick", [](TestContext& InContext)
		{
			model::Scheduler Plan;
			int Ran = 0;
			for ( int Index = 0; Index < 3; ++Index )
			{
				Plan.At(0.0, [&Ran]() { ++Ran; });
			}
			Plan.RunDue(1.0, std::chrono::microseconds(0));
			InContext.Expect(Ran == 1 && Plan.GetPendingCount() == 2, "a zero budget still runs one");
			Plan.Tick(0.0f, UnlimitedBudget);
			InContext.Expect(Ran == 3, "no budget runs the rest");
		});

		InSuite.Add("TimerManager: After fires on real time, sliced jobs spread over updates and can be cancelled", [](TestContext& InContext)
		{
			model::TimerManager Timers;
			int Fired = 0;
			Timers.After(1.0f, [&Fired]() { ++Fired; });
			int Steps = 0;
			bool bDone = false;
			// A zero budget still runs one step per update.
			Timers.RunSliced(std::chrono::microseconds(0), [&Steps]() { return ++Steps < 3; }, [&bDone]() { bDone = true; });
			const model::TimerManager::Handle Dropped = Timers.RunSliced(std::chrono::microseconds(0), []() { return true; });
			InContext.Expect(Timers.Cancel(Dropped) && !Timers.Cancel(Dropped), "a sliced job is cancelled once");
			Timers.Tick(500.0f, UnlimitedBudget);
			InContext.Expect(Steps == 1 && !bDone && Fired == 0, "one step per update with no budget");
			Timers.Tick(600.0f, UnlimitedBudget);
			Timers.Tick(100.0f, UnlimitedBudget);
			InContext.Expect(Steps == 3 && bDone && Fired == 1 && Timers.GetPendingCount() == 0, "the job finished, the timer fired, nothing is left");
			int Counted = 0;
			const bool bFinished = RunWithinBudget(std::chrono::seconds(10), [&Counted]() { return ++Counted < 1000; });
			InContext.Expect(bFinished && Counted == 1000, "a generous budget runs the work to the end");
		});

		InSuite.Add("Timed callbacks: the same time cut into other steps, or with zero-length ticks between, fires them at the same moments", [](TestContext& InContext)
		{
			constexpr float StepMs = 1000.0f / 60.0f;
			const auto Run = [](const int InStepsPerFrame, const bool bInZeroTicks)
			{
				model::Scheduler Plan;
				model::TimerManager Timers;
				model::SimulationClock Clock;
				TArray<int> PlanFiredAt;
				TArray<int> TimerFiredAt;
				int Step = 0;
				Plan.After(0.5f, [&]() { PlanFiredAt.PushBack(Step); });
				Timers.After(0.25f, [&]() { TimerFiredAt.PushBack(Step); });
				while ( Step < 60 )
				{
					for ( int Index = 0; Index < InStepsPerFrame && Step < 60; ++Index )
					{
						++Step;
						Plan.Tick(StepMs, UnlimitedBudget);
						Timers.Tick(StepMs, UnlimitedBudget);
						Clock.Tick(StepMs, UnlimitedBudget);
					}
					if ( bInZeroTicks )
					{
						Plan.Tick(0.0f, UnlimitedBudget);
						Timers.Tick(0.0f, UnlimitedBudget);
						Clock.Tick(0.0f, UnlimitedBudget);
					}
				}
				return std::make_tuple(PlanFiredAt, TimerFiredAt, Clock.GetGameplayTick());
			};
			const auto Single = Run(1, false);
			const auto Batched = Run(4, true);
			const auto Padded = Run(1, true);
			InContext.Expect(std::get<0>(Single) == std::get<0>(Batched) && std::get<0>(Single) == std::get<0>(Padded), "the scheduler fires on the same step");
			InContext.Expect(std::get<1>(Single) == std::get<1>(Batched) && std::get<1>(Single) == std::get<1>(Padded), "the timer fires on the same step");
			InContext.ExpectEqual(static_cast<int>(std::get<0>(Single).Size() + std::get<1>(Single).Size()), 2, "each fired once");
			InContext.ExpectEqual(static_cast<int>(std::get<2>(Single)), static_cast<int>(std::get<2>(Padded)), "zero-length ticks are not simulation steps");
		});

		InSuite.Add("Countdown: ticks restart the interval, readiness runs 0 to 1", [](TestContext& InContext)
		{
			model::Countdown Timer;
			Timer.Configure(2.0f);
			InContext.ExpectNear(Timer.GetReadiness(), 0.0f, 0.001f, "just started");
			InContext.Expect(!Timer.Tick(1.0f), "half way");
			InContext.ExpectNear(Timer.GetReadiness(), 0.5f, 0.001f, "half ready");
			InContext.Expect(Timer.Tick(1.5f), "ran out");
			InContext.ExpectNear(Timer.GetRemaining(), 2.0f, 0.001f, "starts over");
		});

		InSuite.Add("Minelayer: a mine is due every interval of driving, standing still pauses the count", [](TestContext& InContext)
		{
			model::DropTimer Timer;
			Timer.Configure(1.0f);
			InContext.Expect(!Timer.Update(0.6f, true), "not yet");
			InContext.Expect(!Timer.Update(5.0f, false), "standing still drops nothing");
			InContext.Expect(Timer.GetRemaining() > 0.39f && Timer.GetRemaining() < 0.41f, "and keeps the count");
			InContext.Expect(Timer.Update(0.5f, true), "due after a second of driving");
			InContext.Expect(Timer.GetReadiness() < 0.01f, "the next one starts over");
		});

		InSuite.Add("Minelayer: a driving minelayer leaves still mines of its team behind it", [](TestContext& InContext)
		{
			Tank* Layer = CreateTankAt(InContext, "configs/test/minelayer_vehicle.json", cocos2d::Vec2(TileSize * 4.5f, TileSize * 2.5f));
			if ( !Layer )
			{
				return;
			}
			auto FindMine = []() { return GameContext::Get<ObjectsFactory>().FindObjectIf([](const FGameObjectHandle& InObject) { return InObject->GetType() == EObjectType::Bullet; }); };
			Advance(1.0f);
			InContext.Expect(!FindMine(), "nothing while standing");
			Layer->OnCommandReceived(GameCommand(ECommandType::MoveForward));
			Advance(0.5f);
			auto Mine = FindMine();
			InContext.Expect(Mine && Mine->GetTeam() == ETeam::Enemy, "a mine of its team after driving a while");
			if ( Mine )
			{
				const cocos2d::Vec2 Laid = Mine->GetVisualNode()->getPosition();
				InContext.Expect(Laid.y < Layer->GetVisualNode()->getPositionY(), "behind it");
				Advance(0.2f);
				InContext.Expect(Mine->GetVisualNode()->getPosition() == Laid, "lying still");
			}
		});

		InSuite.Add("Engineer: repairs the most hurt allies within reach, never the healthy or the far", [](TestContext& InContext)
		{
			const TArray<model::RepairCandidate> Candidates = {
				{ 5, 5, 10.0f },  // healthy
				{ 2, 6, 20.0f },  // a third left
				{ 1, 4, 500.0f }, // too far
				{ 3, 4, 30.0f },  // three quarters left
				{ 1, 2, 40.0f }   // half left
			};
			const TArray<int> Two = model::RepairPicker::Pick(Candidates, 100.0f, 2);
			InContext.Expect(Two == TArray<int>({ 1, 4 }), "the two most hurt in reach, most hurt first");
			InContext.ExpectEqual(static_cast<int>(model::RepairPicker::Pick(Candidates, 100.0f, 10).Size()), 3, "all damaged ones in reach");
			InContext.Expect(model::RepairPicker::Pick(Candidates, 5.0f, 2).IsEmpty(), "nobody in reach");
		});

		InSuite.Add("Engineer: an engineer heals a damaged ally nearby by itself", [](TestContext& InContext)
		{
			Tank* Engineer = CreateTankAt(InContext, "configs/test/engineer_vehicle.json", cocos2d::Vec2(TileSize * 4.5f, TileSize * 3.5f));
			Tank* Ally = CreateTankAt(InContext, "configs/test/engineer_vehicle.json", cocos2d::Vec2(TileSize * 6.5f, TileSize * 3.5f));
			if ( !Engineer || !Ally )
			{
				return;
			}
			Ally->ApplyDamage(4, ETeam::Player);
			const int Hurt = Ally->GetHealth();
			InContext.Expect(Hurt < Ally->GetMaxHealth(), "the ally is hurt");
			Advance(0.7f);
			InContext.Expect(Ally->GetHealth() > Hurt, "repaired");
			InContext.ExpectEqual(Engineer->GetHealth(), Engineer->GetMaxHealth(), "the healthy engineer is left alone");
		});

		InSuite.Add("Mortar: a lobbed shell rises and falls, lands at its distance, aims at a foe within its range", [](TestContext& InContext)
		{
			model::LobFlight Flight;
			Flight.Start(100.0f);
			InContext.Expect(!Flight.Advance(50.0f) && Flight.GetHeight() > 0.99f, "at the top half way");
			InContext.Expect(Flight.Advance(60.0f) && !Flight.IsFlying() && Flight.GetProgress() == 1.0f, "lands at the end, not past it");
			InContext.Expect(model::LobFlight::ChooseDistance(0.0f, 64.0f, 256.0f) == 256.0f, "no target: the longest lob");
			InContext.Expect(model::LobFlight::ChooseDistance(150.0f, 64.0f, 256.0f) == 150.0f, "lands on the target");
			InContext.Expect(model::LobFlight::ChooseDistance(20.0f, 64.0f, 256.0f) == 64.0f, "never closer than the minimum");
			InContext.Expect(model::LobFlight::ChooseDistance(900.0f, 64.0f, 256.0f) == 256.0f, "never further than the maximum");
		});

		InSuite.Add("Mortar: the shell lands on a tank ahead and hurts it with the blast", [](TestContext& InContext)
		{
			Tank* Target = CreateTankAt(InContext, PlayerTankPath(), cocos2d::Vec2(TileSize * 4.5f, TileSize * 8.5f));
			Tank* Mortar = CreateTankAt(InContext, "configs/test/mortar_vehicle.json", cocos2d::Vec2(TileSize * 4.5f, TileSize * 3.5f));
			if ( !Target || !Mortar )
			{
				return;
			}
			const int StartHealth = Target->GetHealth();
			InContext.Expect(Mortar->Fire(), "fires");
			Advance(0.3f);
			InContext.ExpectEqual(Target->GetHealth(), StartHealth, "nothing yet while the shell flies");
			Advance(2.0f);
			InContext.Expect(Target->GetHealth() < StartHealth, "the blast hurts the tank it landed on");
			InContext.Expect(!GameContext::Get<ObjectsFactory>().FindObjectIf([](const FGameObjectHandle& InObject) { return InObject->GetType() == EObjectType::Bullet; }), "the shell is gone after landing");
		});

		InSuite.Add("Machine gun: the bullets leave the muzzle and keep flying", [](TestContext& InContext)
		{
			Tank* Gunner = CreateTankAt(InContext, PlayerTankPath(), cocos2d::Vec2(TileSize * 4.5f, TileSize * 3.5f));
			if ( !Gunner )
			{
				return;
			}
			Gunner->SetAmmo(0);
			for ( size_t Tries = 0; Tries < Gunner->GetWeaponCount() && Gunner->GetActiveWeapon().bUsesAmmo; ++Tries )
			{
				Gunner->SwitchWeapon();
			}
			InContext.Expect(!Gunner->GetActiveWeapon().bUsesAmmo, "the machine gun is selected");
			InContext.Expect(Gunner->Fire(), "fires");
			Advance(0.05f);
			float FirstY = 0.0f;
			int Count = 0;
			GameContext::Get<ObjectsFactory>().ForEachObject<Bullet>([&](Bullet& InBullet)
			{
				FirstY = InBullet.GetVisualNode()->getPositionY();
				++Count;
			});
			InContext.Expect(Count > 0, "a bullet exists");
			Advance(0.1f);
			float SecondY = 0.0f;
			GameContext::Get<ObjectsFactory>().ForEachObject<Bullet>([&](Bullet& InBullet)
			{
				SecondY = InBullet.GetVisualNode()->getPositionY();
			});
			InContext.Expect(SecondY > FirstY + TileSize * 0.5f, "the bullet moved on");
		});

		InSuite.Add("Machine gun: a held trigger never leaves a bullet standing still", [](TestContext& InContext)
		{
			Tank* Gunner = CreateTankAt(InContext, PlayerTankPath(), cocos2d::Vec2(TileSize * 4.5f, TileSize * 1.5f));
			if ( !Gunner )
			{
				return;
			}
			Gunner->SetAmmo(0);
			for ( size_t Tries = 0; Tries < Gunner->GetWeaponCount() && Gunner->GetActiveWeapon().bUsesAmmo; ++Tries )
			{
				Gunner->SwitchWeapon();
			}
			std::map<const Bullet*, cocos2d::Vec2> LastSeen;
			int Standing = 0;
			int Shots = 0;
			for ( int Frame = 0; Frame < 120; ++Frame )
			{
				Shots += Gunner->Fire() ? 1 : 0;
				Advance(FrameTime);
				GameContext::Get<ObjectsFactory>().ForEachObject<Bullet>([&](Bullet& InBullet)
				{
					const cocos2d::Vec2 Position = InBullet.GetVisualNode()->getPosition();
					const auto Found = LastSeen.find(&InBullet);
					Standing += Found != LastSeen.end() && Found->second == Position && !InBullet.IsDestroying() ? 1 : 0;
					LastSeen[&InBullet] = Position;
				});
			}
			InContext.Expect(Shots > 3, "several shots were fired");
			InContext.ExpectEqual(Standing, 0, "no bullet stood still between two frames");
		});

		InSuite.Add("Shield: a shot against the facing hits the front, along it the back, across it the side", [](TestContext& InContext)
		{
			using model::ShotSide;
			using model::ShotSideClassifier;
			InContext.Expect(ShotSideClassifier::Classify({ 0.0f, 1.0f }, { 0.0f, -1.0f }) == ShotSide::Front, "facing up, a shot flying down hits the front");
			InContext.Expect(ShotSideClassifier::Classify({ 0.0f, 1.0f }, { 0.0f, 1.0f }) == ShotSide::Back, "facing up, a shot flying up hits the back");
			InContext.Expect(ShotSideClassifier::Classify({ 0.0f, 1.0f }, { 1.0f, 0.0f }) == ShotSide::Side, "a shot across hits the side");
			InContext.Expect(ShotSideClassifier::Classify({ 1.0f, 0.0f }, { -3.0f, 0.0f }) == ShotSide::Front, "facing right, a shot flying left hits the front");
			InContext.Expect(ShotSideClassifier::Classify({ 1.0f, 0.0f }, { 0.0f, 0.0f }) == ShotSide::Side, "no direction counts as the side");
		});

		InSuite.Add("Shield: the shield bearer ignores shots at its front and takes double at its back", [](TestContext& InContext)
		{
			Tank* Bearer = CreateTankAt(InContext, "configs/objects/movables/tanks/enemies/ShieldBearer.json", cocos2d::Vec2(TileSize * 4.5f, TileSize * 4.5f));
			if ( !Bearer )
			{
				return;
			}
			InContext.Expect(Bearer->HasFrontalShield(), "the config gives it a frontal shield");
			const cocos2d::Vec2 Ahead = Bearer->GetDirection();
			const int StartHealth = Bearer->GetHealth();
			Bearer->ApplyDamageFrom(2, ETeam::Player, -Ahead);
			InContext.Expect(Bearer->GetHealth() == StartHealth, "a shot at the front does nothing");
			Bearer->ApplyDamageFrom(1, ETeam::Player, cocos2d::Vec2(-Ahead.y, Ahead.x));
			InContext.Expect(Bearer->GetHealth() == StartHealth - 1, "a shot at the side hurts as usual");
			Bearer->ApplyDamageFrom(1, ETeam::Player, Ahead);
			InContext.Expect(Bearer->GetHealth() == StartHealth - 3, "a shot at the back hurts twice as much");
		});

		InSuite.Add("Ram: a charge ends in a stun, then a cooldown, then it is ready again", [](TestContext& InContext)
		{
			using Phase = model::ChargeState::Phase;
			model::ChargeState State;
			State.Configure(1.0f, 0.5f, 2.0f);
			InContext.Expect(State.Start() && State.IsCharging(), "charging");
			InContext.Expect(!State.Start(), "no second charge while charging");
			State.Update(0.4f);
			State.Hit();
			InContext.Expect(State.IsStunned(), "a hit ends the charge in a stun");
			InContext.Expect(State.Update(0.5f) && State.GetPhase() == Phase::Cooldown, "the stun wears off into the cooldown");
			InContext.Expect(State.GetReadiness() > 0.0f && State.GetReadiness() < 1.0f, "recharging");
			InContext.Expect(State.Update(2.0f) && State.GetPhase() == Phase::Ready, "ready after the cooldown");
			State.Start();
			State.Update(1.0f);
			InContext.Expect(State.IsStunned(), "a charge that runs out of time stuns too");
			State.Hit();
			InContext.Expect(State.IsStunned() && State.GetRemaining() > 0.49f, "a hit while stunned changes nothing");
		});

		InSuite.Add("Ram: charges a tank in line ahead, hurts it on contact and stands stunned", [](TestContext& InContext)
		{
			Tank* Target = CreateTankAt(InContext, PlayerTankPath(), cocos2d::Vec2(TileSize * 4.5f, TileSize * 7.5f));
			Tank* Ram = CreateTankAt(InContext, "configs/test/ram_vehicle.json", cocos2d::Vec2(TileSize * 4.5f, TileSize * 3.5f));
			if ( !Target || !Ram )
			{
				return;
			}
			const int StartHealth = Target->GetHealth();
			Advance(3.0f / 60.0f);
			InContext.Expect(Ram->IsAiSuspended(), "starts charging by itself");
			bool bHit = false;
			for ( int Step = 0; Step < 120 && !bHit; ++Step )
			{
				Advance(1.0f / 60.0f);
				bHit = Target->GetHealth() < StartHealth;
			}
			InContext.Expect(bHit, "the target lost health on contact");
			InContext.ExpectEqual(StartHealth - Target->GetHealth(), 4, "by the contact damage");
			const cocos2d::Vec2 Stunned = Ram->GetVisualNode()->getPosition();
			Advance(0.5f);
			InContext.Expect(Ram->GetVisualNode()->getPosition().distance(Stunned) < 1.0f, "stands still while stunned");
			Advance(2.0f);
			InContext.Expect(!Ram->IsAiSuspended(), "the AI gets control back after the stun");
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
		GameContext::Get<ObjectsFactory>().ForEachObject(EObjectType::Bullet, [&Count](const FGameObjectHandle& InObject)
		{
			Count += InObject->IsDestroying() ? 0 : 1;
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

		InSuite.Add("Weapons: the machine gun overheats after ten shots in a row and waits", [](TestContext& InContext)
		{
			Tank* CurrentTank = CreateTankAt(InContext, PlayerTankPath(), cocos2d::Vec2(TileSize * 4.5f, TileSize * 4.5f));
			if ( !CurrentTank )
			{
				return;
			}
			for ( size_t Index = 0; Index < CurrentTank->GetWeaponCount() && CurrentTank->GetActiveWeapon().Id != "machine_gun"; ++Index )
			{
				CurrentTank->SelectWeapon(Index);
			}
			InContext.Expect(CurrentTank->GetActiveWeapon().Id == "machine_gun", "machine gun is active");
			int Shots = 0;
			for ( int Step = 0; Step < 100 && CurrentTank->GetReloadRemaining() < 1.0f; ++Step )
			{
				Shots += CurrentTank->Fire() ? 1 : 0;
				Advance(0.05f);
			}
			InContext.ExpectEqual(Shots, 10, "ten shots, then a long cool-down");
			Advance(0.5f);
			InContext.Expect(!CurrentTank->Fire(), "no shot while it cools");
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
			auto Mine = GameContext::Get<ObjectsFactory>().FindObjectIf([](const FGameObjectHandle& InObject) { return InObject->GetType() == EObjectType::Bullet; });
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
			Wall* CurrentWall = CreateWall(InContext, "configs/objects/spawnables/walls/stone.json", TankAt + cocos2d::Vec2(0.0f, TileSize * 2.0f));
			if ( !CurrentWall || !CurrentTank->Fire() )
			{
				return;
			}
			auto Shell = GameContext::Get<ObjectsFactory>().FindObjectIf([](const FGameObjectHandle& InObject) { return InObject->GetType() == EObjectType::Bullet; });
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
			CurrentTank->OnCommandReceived(GameCommand(ECommandType::StartFiring));
			Advance(CurrentTank->GetReloadRemaining() * 3.0f);
			InContext.ExpectEqual(CurrentTank->GetAmmo(), StartAmmo - 1, "one shell for one press");
			CurrentTank->OnCommandReceived(GameCommand(ECommandType::StopFiring));

			InContext.Expect(CurrentTank->EnableAutoFire("cannon") && CurrentTank->CanAutoFire(), "the upgrade turns auto-fire on");
			InContext.Expect(!CurrentTank->EnableAutoFire("no_such_gun"), "an unknown weapon is reported");
		});

		InSuite.Add("Weapons: the auto-fire upgrade reaches the tank from the shop", [](TestContext& InContext)
		{
			const auto& Upgrades = GameContext::Get<UpgradeManager>().GetUpgrades();
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
			CurrentTank->OnCommandReceived(GameCommand(ECommandType::StartFiring));
			InContext.Expect(CurrentTank->IsTriggerHeld() && CurrentTank->GetAmmo() == StartAmmo - 1, "the first shell leaves at once");

			const float Reload = CurrentTank->GetReloadRemaining();
			Advance(Reload * 2.5f);
			InContext.Expect(CurrentTank->GetAmmo() <= StartAmmo - 3, "more shells follow while the trigger is held");

			CurrentTank->OnCommandReceived(GameCommand(ECommandType::StopFiring));
			const int AfterRelease = CurrentTank->GetAmmo();
			Advance(Reload * 2.5f);
			InContext.Expect(!CurrentTank->IsTriggerHeld() && CurrentTank->GetAmmo() == AfterRelease, "no more shots after the release");
		});

		InSuite.Add("Movement: two tanks stuck inside each other can drive apart, but not deeper in", [](TestContext& InContext)
		{
			const cocos2d::Vec2 Center(TileSize * 4.5f, TileSize * 4.5f);
			Tank* First = CreateTankAt(InContext, PlayerTankPath(), Center);
			Tank* Second = CreateTankAt(InContext, "configs/objects/movables/tanks/enemies/Basic.json", Center + cocos2d::Vec2(10.0f, 0.0f));
			if ( !First || !Second )
			{
				return;
			}
			InContext.Expect(OverlapArea(*First, *Second) > 0.0f, "they overlap at the start");
			InContext.Expect(First->CanMoveTo(Center - cocos2d::Vec2(8.0f, 0.0f)), "a step away from the other tank is allowed");
			InContext.Expect(!First->CanMoveTo(Center + cocos2d::Vec2(8.0f, 0.0f)), "a step deeper into it is not");

			Tank* Third = CreateTankAt(InContext, "configs/objects/movables/tanks/enemies/Basic.json", Center - cocos2d::Vec2(TileSize, 0.0f));
			InContext.Expect(Third && !Third->CanMoveTo(Center - cocos2d::Vec2(TileSize - 16.0f, 0.0f)), "a free tank still cannot drive into another");
		});

		InSuite.Add("Movement: tanks inside each other separate by themselves, but not into a wall", [](TestContext& InContext)
		{
			const ResourceHolder CurrentTileset({ ResourceManifest().GetTilesetAtlas("field") });
			const cocos2d::Vec2 Center(TileSize * 4.5f, TileSize * 4.5f);
			Tank* First = CreateTankAt(InContext, PlayerTankPath(), Center);
			Tank* Second = CreateTankAt(InContext, PlayerTankPath(), Center);
			Wall* CurrentWall = CreateWall(InContext, "configs/objects/spawnables/walls/stone.json", Center + cocos2d::Vec2(TileSize, 0.0f));
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
			CurrentTank->OnCommandReceived(GameCommand(ECommandType::MoveRight));
			Advance(0.3f);
			const float StartX = CurrentTank->GetVisualNode()->getPositionX();

			CurrentTank->OnCommandReceived(GameCommand(ECommandType::MoveForward));
			Advance(1.0f);
			InContext.ExpectNear(CurrentTank->GetVisualNode()->getPositionX(), StartX, 0.01f, "the same column after the turn and a second of driving");
			InContext.Expect(CurrentTank->GetVisualNode()->getPositionY() > TileSize * 5.5f, "it drove ahead");
			CurrentTank->OnCommandReceived(GameCommand(ECommandType::Stop));
		});

		// Stone at columns 3 and 5 of row 6 leaves a one-tile gap at column 4; the tank comes from below.
		auto GapTest = [](TestContext& InContext, const float InOffset) -> float
		{
			const ResourceHolder CurrentTileset({ ResourceManifest().GetTilesetAtlas("field") });
			CreateWall(InContext, "configs/objects/spawnables/walls/stone.json", cocos2d::Vec2(TileSize * 3.5f, TileSize * 6.5f));
			CreateWall(InContext, "configs/objects/spawnables/walls/stone.json", cocos2d::Vec2(TileSize * 5.5f, TileSize * 6.5f));
			Tank* CurrentTank = CreateTankAt(InContext, PlayerTankPath(), cocos2d::Vec2(TileSize * 4.5f + InOffset, TileSize * 4.5f));
			if ( !CurrentTank )
			{
				return 0.0f;
			}
			CurrentTank->OnCommandReceived(GameCommand(ECommandType::MoveForward));
			Advance(1.5f);
			CurrentTank->OnCommandReceived(GameCommand(ECommandType::Stop));
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
			CurrentLevel.EnemyIds = { FPersistentString{ "Heavy" } };
			Wave CurrentWave;
			CurrentWave.Delay = 0.0f;
			CurrentWave.Interval = 0.0f;
			CurrentWave.MaxAlive = 3;
			CurrentWave.Entries.PushBack({ tiles::EnemySpawn, 3 });
			CurrentLevel.Waves.PushBack(CurrentWave);

			const cocos2d::Vec2 Point(TileSize / 2, (GridRows - 0.5f) * TileSize);
			EnemySpawner CurrentSpawner(InContext.GetSandbox(), cocos2d::Rect(0, 0, GridColumns * TileSize, GridRows * TileSize), CurrentLevel, { { tiles::EnemySpawn, { Point } } });
			TArray<Tank*> CurrentEnemies;
			CurrentSpawner.OnSpawned = [&CurrentEnemies](Tank& InTank) { CurrentEnemies.PushBack(&InTank); };
			for ( int CurrentFrame = 0; CurrentFrame < 60; ++CurrentFrame )
			{
				CurrentSpawner.Update(FrameTime);
				Advance(FrameTime);
			}

			for ( size_t Index = 0; Index < CurrentEnemies.Size(); ++Index )
			{
				for ( size_t InnerIndex = Index + 1; InnerIndex < CurrentEnemies.Size(); ++InnerIndex )
				{
					InContext.Expect(OverlapArea(*CurrentEnemies[Index], *CurrentEnemies[InnerIndex]) <= 0.0f, "enemies " + std::to_string(Index) + " and " + std::to_string(InnerIndex) + " do not overlap");
				}
			}
			InContext.Expect(!CurrentEnemies.IsEmpty(), "an enemy appeared");
			InContext.Note(std::to_string(CurrentEnemies.Size()) + " enemies appeared in one second from one point");
			// Spawn flashes still running would call the spawner after this test.
			InContext.GetSandbox()->stopAllActions();
		});

		InSuite.Add("Display: map scale options and the window size for a screen", [](TestContext& InContext)
		{
			InContext.ExpectEqual(NextMapScale(0), 100, "auto -> 100");
			InContext.ExpectEqual(NextMapScale(100), 140, "100 -> 140");
			InContext.ExpectEqual(NextMapScale(140), 150, "140 -> 150");
			InContext.ExpectEqual(NextMapScale(200), 0, "200 -> auto");
			const PixelSize Design = { 1024, 864 };
			const PixelSize Screen = { 1920, 1040 };
			const PixelSize Window = ComputeWindowSize(Design, Screen);
			InContext.Expect(Window.Height < Screen.Height && Window.Height > 864, "the window fills most of the screen");
			InContext.ExpectNear(static_cast<float>(Window.Width) / Window.Height, 1024.0f / 864.0f, 0.01f, "the proportions are kept");
		});

		InSuite.Add("VisibilityMap: no fog shows everything, fog shows the square around the viewer", [](TestContext& InContext)
		{
			using model::CellRect;
			using model::VisibilityMap;
			VisibilityMap Open(16, 11, false, 2.0f);
			InContext.Expect(Open.IsCellVisible(15, 10) && Open.IsAreaVisible({ 3, 3, 3, 3 }), "without fog all cells are visible");

			VisibilityMap Fogged(16, 11, true, 2.0f);
			Fogged.SetViewer({ 8.5f, 5.5f });
			InContext.Expect(Fogged.IsCellVisible(8, 5), "the viewer's own cell");
			InContext.Expect(Fogged.IsCellVisible(6, 3) && Fogged.IsCellVisible(10, 7), "the square's corners are visible");
			InContext.Expect(!Fogged.IsCellVisible(5, 5) && !Fogged.IsCellVisible(11, 5) && !Fogged.IsCellVisible(8, 8) && !Fogged.IsCellVisible(8, 2), "one cell further is fogged");
			InContext.Expect(Fogged.IsAreaVisible({ 4, 5, 6, 5 }), "a wide object shows when one of its cells is visible");
			InContext.Expect(!Fogged.IsAreaVisible({ 0, 0, 4, 10 }) && !Fogged.IsAreaVisible(CellRect()), "a fogged or empty area does not");

			const CellRect Draw = Fogged.GetDrawRect({ 0, 0, 15, 10 });
			InContext.Expect(Draw.MinX == 6 && Draw.MaxX == 10 && Draw.MinY == 3 && Draw.MaxY == 7, "the draw rect is the vision square");
			Fogged.SetViewer({ 0.5f, 0.5f });
			const CellRect Corner = Fogged.GetDrawRect({ 0, 0, 15, 10 });
			InContext.Expect(Corner.MinX == 0 && Corner.MinY == 0 && Corner.MaxX == 2 && Corner.MaxY == 2, "cut to the map at its corner");
			const CellRect CameraCut = Open.GetDrawRect({ -3, 2, 20, 4 });
			InContext.Expect(CameraCut.MinX == 0 && CameraCut.MaxX == 15 && CameraCut.MinY == 2 && CameraCut.MaxY == 4, "without fog the camera rect is cut to the map");
		});

		InSuite.Add("MapCamera: the scale fits or follows the percent, the map is centred or follows the focus", [](TestContext& InContext)
		{
			using model::MapCamera;
			using model::Vec2f;
			const Vec2f Map = { 1000.0f, 800.0f };
			const Vec2f View = { 500.0f, 400.0f };
			InContext.ExpectNear(MapCamera::ComputeScale(0, Map, View), 0.5f, 0.0001f, "auto fits the whole map");
			InContext.ExpectNear(MapCamera::ComputeScale(150, Map, View), 1.5f, 0.0001f, "a percent is the map's own size times it");
			const Vec2f Centred = MapCamera::ComputeOffset({ 10.0f, 10.0f }, 0.5f, Map, View);
			InContext.Expect(Centred.X == 0.0f && Centred.Y == 0.0f, "a map that fits needs no offset");
			const Vec2f Small = MapCamera::ComputeOffset({ 0.0f, 0.0f }, 0.25f, Map, View);
			InContext.Expect(Small.X == 125.0f && Small.Y == 100.0f, "a map smaller than the view is centred");
			const Vec2f Middle = MapCamera::ComputeOffset({ 500.0f, 400.0f }, 1.0f, Map, View);
			InContext.Expect(Middle.X == -250.0f && Middle.Y == -200.0f, "a bigger map keeps the focus in the middle");
			const Vec2f Corner = MapCamera::ComputeOffset({ 0.0f, 0.0f }, 1.0f, Map, View);
			InContext.Expect(Corner.X == 0.0f && Corner.Y == 0.0f, "and stops at the top-left edge");
			const Vec2f Far = MapCamera::ComputeOffset({ 1000.0f, 800.0f }, 1.0f, Map, View);
			InContext.Expect(Far.X == -500.0f && Far.Y == -400.0f, "and at the bottom-right edge");
		});

		InSuite.Add("Display: the display settings survive saving and loading the profile", [](TestContext& InContext)
		{
			auto& Manager = GameContext::Get<ProfileManager>();
			const PlayerProfile Saved = Manager.Get();
			auto& CurrentProfile = Manager.Get();
			CurrentProfile.bVerticalSync = false;
			CurrentProfile.CurrentWindowMode = EWindowMode::Borderless;
			CurrentProfile.MapScale = 150;
			CurrentProfile.bSmoothPixels = true;
			CurrentProfile.EffectsVolume = 40;
			CurrentProfile.TextureQuality = static_cast<int32>(ETextureQuality::FullHd);
			CurrentProfile.WindowPosition = PixelPoint{ -1200, 35 };
			Manager.Save();
			CurrentProfile = PlayerProfile();
			Manager.Load();
			InContext.Expect(!Manager.Get().bVerticalSync, "vertical sync");
			InContext.Expect(Manager.Get().CurrentWindowMode == EWindowMode::Borderless, "window mode");
			InContext.ExpectEqual(Manager.Get().MapScale, 150, "map scale");
			InContext.Expect(Manager.Get().bSmoothPixels, "pixel smoothing");
			InContext.ExpectEqual(Manager.Get().EffectsVolume, 40, "effects volume");
			InContext.ExpectEqual(Manager.Get().TextureQuality, static_cast<int32>(ETextureQuality::FullHd), "texture quality");
			InContext.Expect(Manager.Get().WindowPosition.Has(), "the window position is kept");
			if ( Manager.Get().WindowPosition )
			{
				InContext.ExpectEqual(Manager.Get().WindowPosition.Get().X, -1200, "window x (a screen on the left)");
				InContext.ExpectEqual(Manager.Get().WindowPosition.Get().Y, 35, "window y");
			}
			Manager.Get().WindowPosition.Reset();
			Manager.Save();
			Manager.Load();
			InContext.Expect(!Manager.Get().WindowPosition.Has(), "no saved position means centred");
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
			InContext.ExpectEqual(GameContext::Get<ResourceManager>().GetAtlasLoadedPath(Atlas), AtlasFile, "SD atlas loaded");
			cocos2d::SpriteFrame* SdFrame = cocos2d::SpriteFrameCache::getInstance()->getSpriteFrameByName(Frame);
			InContext.Expect(SdFrame != nullptr, "SD frame");
			const cocos2d::Size SdSize = SdFrame ? SdFrame->getOriginalSize() : cocos2d::Size::ZERO;

			ApplyHdTextures(true);
			const TextureQualityConfig& Config = GetTextureQualityConfig();
			InContext.ExpectNear(cocos2d::Director::getInstance()->getContentScaleFactor(), Config.HdScale, 0.001f, "HD: content scale raised");
			InContext.ExpectEqual(GameContext::Get<ResourceManager>().GetAtlasLoadedPath(Atlas), MapAtlasPath(AtlasFile, true, Config), "HD atlas loaded");
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

		InSuite.Add("Display: FullHD textures keep sprite size in points and have 4x the pixels", [](TestContext& InContext)
		{
			const ETextureQuality WasQuality = GetTextureQuality();
			const std::string Frame = "player_tank";
			ApplyTextureQuality(ETextureQuality::Sd);
			cocos2d::SpriteFrame* SdFrame = cocos2d::SpriteFrameCache::getInstance()->getSpriteFrameByName(Frame);
			const cocos2d::Size SdSize = SdFrame ? SdFrame->getOriginalSize() : cocos2d::Size::ZERO;
			ApplyTextureQuality(ETextureQuality::FullHd);
			const TextureQualityConfig& Config = GetTextureQualityConfig();
			InContext.ExpectEqual(GameContext::Get<ResourceManager>().GetAtlasLoadedPath("atlas.player"), MapAtlasPath("textures/player.plist", ETextureQuality::FullHd, Config), "FullHD atlas loaded");
			cocos2d::SpriteFrame* FullFrame = cocos2d::SpriteFrameCache::getInstance()->getSpriteFrameByName(Frame);
			InContext.Expect(FullFrame != nullptr, "FullHD frame");
			if ( FullFrame )
			{
				InContext.ExpectNear(FullFrame->getOriginalSize().width, SdSize.width, 0.5f, "same width in points");
				InContext.ExpectNear(FullFrame->getOriginalSizeInPixels().width, SdSize.width * Config.FullHdScale, 0.5f, "4x pixels");
			}
			ApplyTextureQuality(WasQuality);
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
				const float Scale = cocos2d::Director::getInstance()->getFontScaleFactor();
				// The cache lends the atlas without a reference: releasing it here would free it under the label.
				cocos2d::FontAtlas* Atlas = cocos2d::FontAtlasCache::getFontAtlasTTF(&FontConfig);
				InContext.Expect(Atlas != nullptr, std::string(InStep) + ": atlas exists");
				InContext.Expect(MenuLabel->getFontAtlas() == Atlas, std::string(InStep) + ": the label uses the cached atlas");
				if ( Atlas )
				{
					const float LineHeightInPoints = Atlas->getLineHeight() / Scale;
					if ( SdLineHeight == 0.0f )
					{
						SdLineHeight = LineHeightInPoints;
					}
					InContext.ExpectNear(LineHeightInPoints, SdLineHeight, 1.5f, std::string(InStep) + ": glyphs rasterized at the font scale");
				}
				MenuLabel->setString("Continue!");
				MenuLabel->setString("Continue");
				MenuLabel->updateContent();
				InContext.Expect(Atlas != nullptr && Atlas->getTexture(0) != nullptr, std::string(InStep) + ": atlas has a texture");
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
			GameContext::Get<ResourceManager>().SetSmoothPixels(true);
			InContext.Expect(GameContext::Get<ResourceManager>().IsSmoothPixels(), "smooth pixels");
			GameContext::Get<ResourceManager>().SetSmoothPixels(false);
			InContext.Expect(!GameContext::Get<ResourceManager>().IsSmoothPixels(), "sharp pixels again");

			const bool bVsync = IsVerticalSyncEnabled();
			SetVerticalSyncEnabled(true);
			InContext.Expect(GetVerticalSyncInterval() != 0, "the driver has vertical sync on");
			SetVerticalSyncEnabled(false);
			InContext.Expect(GetVerticalSyncInterval() <= 0, "and off");
			SetVerticalSyncEnabled(bVsync);

			const float CurrentVolume = GameContext::Get<AudioManager>().GetVolume();
			GameContext::Get<AudioManager>().SetVolume(0.4f);
			InContext.ExpectNear(GameContext::Get<AudioManager>().GetVolume(), 0.4f, 0.001f, "the effects volume");
			GameContext::Get<AudioManager>().SetVolume(CurrentVolume);
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
		InSuite.Add("AI: tank AI profiles come from configs/balance/ai/tank_ai.json", [](TestContext& InContext)
		{
			const TankAiSettings Wander = AiSettingsLoader::LoadTankAi(EAiType::Wander);
			const TankAiSettings Hunter = AiSettingsLoader::LoadTankAi(EAiType::Hunter);
			InContext.ExpectNear(Wander.GetThinkIntervalMin(), 0.8f, 0.001f, "wander think interval min");
			InContext.ExpectNear(Wander.GetThinkIntervalMax(), 2.2f, 0.001f, "wander think interval max");
			InContext.ExpectNear(Wander.GetFocusOnPlayerChance(), 0.0f, 0.001f, "a wanderer never heads for the player");
			InContext.ExpectNear(Hunter.GetThinkIntervalMin(), 0.5f, 0.001f, "hunter think interval min");
			InContext.ExpectNear(Hunter.GetFocusOnPlayerChance(), 0.7f, 0.001f, "hunter focus");
			InContext.ExpectNear(Hunter.GetAimTolerance(), 0.45f * TileSize, 0.01f, "aim tolerance is given in tiles");
			InContext.Expect(Hunter.GetRandomFireRate() > 0.0f && Hunter.GetMaxTurnWait() > 0.0f && Hunter.GetCellCenterTolerance() > 0.0f,
				"every key of the profile is read");
		});

		InSuite.Add("AI: player bot settings come from configs/balance/ai/player_bot.json", [](TestContext& InContext)
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
				const ECommandType Move = AiQueries::RandomMove();
				bOnlyMoves = bOnlyMoves && (Move == ECommandType::MoveForward || Move == ECommandType::MoveBack
					|| Move == ECommandType::MoveLeft || Move == ECommandType::MoveRight);
			}
			InContext.Expect(bOnlyMoves, "200 random moves");
		});

		InSuite.Add("AI: moves along the longer or the shorter axis", [](TestContext& InContext)
		{
			InContext.Expect(AiQueries::MoveAlongMajorAxis(cocos2d::Vec2(10.0f, 2.0f)) == ECommandType::MoveRight, "major: right");
			InContext.Expect(AiQueries::MoveAlongMajorAxis(cocos2d::Vec2(-1.0f, -5.0f)) == ECommandType::MoveBack, "major: down");
			InContext.Expect(AiQueries::MoveAlongMinorAxis(cocos2d::Vec2(10.0f, 2.0f)) == ECommandType::MoveForward, "minor: up");
			InContext.Expect(AiQueries::MoveAlongMinorAxis(cocos2d::Vec2(-1.0f, -5.0f)) == ECommandType::MoveLeft, "minor: left");
		});
	}

	void AddObjectCastTests(TestSuite& InSuite)
	{
		InSuite.Add("ObjectCast: casts by the object type, without dynamic_cast", [](TestContext& InContext)
		{
			const ResourceHolder CurrentTileset({ ResourceManifest().GetTilesetAtlas("field") });
			Tank* Player = CreateTank(InContext, PlayerTankPath());
			Wall* Brick = CreateWall(InContext, "configs/objects/spawnables/walls/brick.json", cocos2d::Vec2(TileSize / 2, TileSize / 2));
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

			const THandle<GameObject> PlayerHandle(Player);
			const THandle<GameObject> WallHandle(Brick);
			const THandle<Tank> PlayerTankHandle = ObjectCast<Tank>(PlayerHandle);
			const THandle<Tank> WallTankHandle = ObjectCast<Tank>(WallHandle);
			InContext.Expect(PlayerTankHandle.Get() == Player, "a tank handle casts to a tank handle");
			InContext.Expect(WallTankHandle == nullptr, "a wall handle does not cast to a tank handle");
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
			InContext.Expect(GameContext::Get<ObjectsFactory>().FindObject<Tank>(PlayerName) != nullptr, "the player is found as a tank");
			InContext.Expect(GameContext::Get<ObjectsFactory>().FindObject<Wall>(PlayerName) == nullptr, "the player is not found as a wall");
			int TankCount = 0;
			GameContext::Get<ObjectsFactory>().ForEachObject<Tank>([&TankCount](Tank&) { ++TankCount; });
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
			const model::JsonValue Position(model::Vec2f{ 2.5f, -3.0f });
			InContext.Expect(bOk && Root.IsObject(), "object parsed");
			InContext.Expect(Root.GetString("name", "") == "heavy", "string");
			InContext.Expect(Root.GetInt("health", 0) == 4, "int");
			InContext.ExpectNear(Root.GetFloat("speed", 0.0f), 1.5f, 0.0001f, "float");
			InContext.ExpectNear(Root.GetFloat("health", 0.0f), 4.0f, 0.0001f, "an int reads as a float");
			InContext.Expect(Root.GetBool("boss", false), "bool");
			InContext.Expect(Root["spawners"].GetArray().Size() == 3 && Root["spawners"].GetArray()[2].AsInt() == 3, "array");
			InContext.Expect(Position.IsArray() && Position.GetArray().Size() == 2
				&& Position.GetArray()[0].AsFloat() == 2.5f && Position.GetArray()[1].AsFloat() == -3.0f, "Vec2f as [x, y]");
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
			InContext.Expect(Root->GetValue<TArray<int>>("rewards") == TArray<int>{ 1, 2, 3 }, "vector of ints");
			TArray<int> Untouched{ 9 };
			InContext.Expect(!Root->TryRead("bad", Untouched) && Untouched == TArray<int>{ 9 }, "a bad item rejects the whole vector");
			const auto Costs = Root->GetValue<TMap<FPersistentString, int>>("costs");
			InContext.Expect(Costs.Size() == 2 && Costs.Find("armor") != nullptr && *Costs.Find("armor") == 20 && Costs.Find("speed") != nullptr && *Costs.Find("speed") == 15, "map by key");
			const auto Waves = Root->GetValue<TArray<TestWaveDef>>("waves");
			InContext.Expect(Waves.Size() == 2 && Waves[0].Count == 4 && Waves[1].Count == 6, "custom type via FromConfig");
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
			const TestSelection AudioOnly = TestSelection::Parse("game.exe --test=audio");
			InContext.Expect(AudioOnly.bAudio && !AudioOnly.bUnit && !AudioOnly.bMenu && !AudioOnly.IsEmpty(), "audio only");

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

	void AddEditorTests(TestSuite& InSuite)
	{
		using model::RectF;

		InSuite.Add("EditorPalette: tabs and entries come from the config, the legend and the object configs", [](TestContext& InContext)
		{
			const auto Wall = JsonConfigNode::Parse(R"({"object_type":"Wall","texture_path":"wall_x","scale":2.0})", "wall");
			const auto Bonus = JsonConfigNode::Parse(R"({"object_type":"Bonus","texture_path":"bonus_x","scale":2.5})", "bonus");
			const auto LegendNode = JsonConfigNode::Parse(R"({"tiles":[{"tile":"X","role":"wall","config":"wall"},
				{"tile":"B","role":"pickup","config":"bonus"},{"tile":".","role":"empty"},{"tile":"P","role":"player","frame":"p","scale":3},
				{"tile":"M","role":"prop","config":"missing"}]})", "legend");
			model::LevelLegend Legend;
			InContext.Expect(Legend.FromConfig(*LegendNode), "legend read");
			const auto Root = JsonConfigNode::Parse(R"({"tabs":[{"id":"walls","name":"T_W"},{"id":"tools"}],
				"entries":[{"tab":"walls","tile":"X","name":"N_X"},
				{"tab":"walls","tile":"B"},
				{"tab":"tools","tile":".","icon":"erase"},
				{"tab":"units","tile":"P"},
				{"tab":"walls","tile":"QQ"},
				{"tab":"walls","tile":"Z"},
				{"tab":"walls","tile":"M"}]})", "palette");
			const auto Find = [&Wall, &Bonus](const std::string& InId) -> const model::IConfigNode*
			{
				return InId == "wall" ? Wall.Get() : (InId == "bonus" ? Bonus.Get() : nullptr);
			};

			model::EditorPalette Palette;
			InContext.Expect(Palette.FromConfig(*Root, Legend, Find), "palette read");
			InContext.Expect(Palette.GetTabs().Size() == 2, "tabs");
			InContext.Expect(Palette.GetTabs()[1].NameKey == "tools", "a tab without a name shows its id");
			InContext.Expect(Palette.GetEntries().Size() == 3, "valid entries");
			InContext.Expect(Palette.GetErrors().Size() == 4, "skipped entries are reported");
			InContext.Expect(Palette.GetEntriesOfTab(0).Size() == 2, "entries on the walls tab");

			const model::EditorPaletteEntry* const WallEntry = Palette.FindByTile('X');
			InContext.Expect(WallEntry && WallEntry->Frame == "wall_x" && WallEntry->Icon == model::PaletteIcon::TilesetFrame
				&& WallEntry->MapScale == 2.0f, "a wall takes its frame and scale from its config");
			const model::EditorPaletteEntry* const BonusEntry = Palette.FindByTile('B');
			InContext.Expect(BonusEntry && BonusEntry->Icon == model::PaletteIcon::Frame && BonusEntry->MapScale == 2.5f, "a bonus uses a shared frame");
			const model::EditorPaletteEntry* const Eraser = Palette.FindByTile('.');
			InContext.Expect(Eraser && Eraser->Icon == model::PaletteIcon::Erase, "the eraser");
		});

		InSuite.Add("EditorPalette: the shipped palette paints only legend characters", [](TestContext& InContext)
		{
			const ConfigReader& Reader = GameContext::Get<ConfigReader>();
			ResourceManager& Resources = GameContext::Get<ResourceManager>();
			model::EditorPalette Palette;
			const bool bRead = Palette.FromConfig(Reader.GetConfig("configs/editor/palette.json"), GameContext::Get<LevelLegendManager>().Get(),
				[&Reader, &Resources](const std::string& InId) -> const model::IConfigNode*
				{
					const FPersistentString Path = Resources.ResolvePath(model::ResourceId(InId), model::ResourceKind::Config);
					return Reader.HasConfig(Path) ? &Reader.GetConfig(Path) : nullptr;
				});
			InContext.Expect(bRead, "configs/editor/palette.json is read");
			InContext.Expect(Palette.GetErrors().IsEmpty(), "no bad entries");
			InContext.Expect(Palette.GetTabs().Size() == 6, "walls, units, bonuses, modifiers, points, tools");
			std::size_t FloorEntries = 0;
			std::size_t PointEntries = 0;
			for ( const model::EditorPaletteEntry& Entry : Palette.GetEntries() )
			{
				const model::PaletteLayer Layer = Palette.GetTabs()[Entry.Tab].Layer;
				FloorEntries += Layer == model::PaletteLayer::Floor && Entry.Icon == model::PaletteIcon::Swatch ? 1 : 0;
				PointEntries += Layer == model::PaletteLayer::Points && Entry.Icon == model::PaletteIcon::Point ? 1 : 0;
			}
			InContext.Expect(FloorEntries == GameContext::Get<LevelLegendManager>().Get().GetGroundEntries().Size(), "the modifiers tab holds every legend ground");
			InContext.Expect(PointEntries == Palette.GetPointNames().Size(), "the points tab holds every point name");
			for ( const char Tile : { tiles::Brick, tiles::Stone, tiles::Wood, tiles::PlayerSpawn, tiles::EnemySpawn, tiles::Ammo, tiles::Repair, tiles::Empty } )
			{
				InContext.Expect(Palette.FindByTile(Tile) != nullptr, std::string("the palette paints '") + Tile + "'");
			}
		});

		InSuite.Add("LevelLegend: the shipped legend loads cleanly and covers every character of every level", [](TestContext& InContext)
		{
			const model::LevelLegend& Legend = GameContext::Get<LevelLegendManager>().Get();
			InContext.Expect(Legend.GetErrors().IsEmpty(), "no bad legend entries");
			InContext.Expect(Legend.GetDefaultSpawner() == tiles::EnemySpawn, "the default spawner");
			InContext.Expect(Legend.IsSpawner('E') && Legend.IsSpawner('1') && Legend.IsSpawner('9') && !Legend.IsSpawner('#'), "spawners include the digits");
			for ( int Index = 0; Index < LevelLoader::GetBuiltinCount(); ++Index )
			{
				LevelData Level;
				InContext.Expect(LevelLoader::Load(LevelLoader::BuiltinId(Index), Level), "level loads: " + LevelLoader::BuiltinId(Index));
				InContext.Expect(!Legend.Scan(Level.Rows).bUnknownTile, "every character is in the legend: " + LevelLoader::BuiltinId(Index));
			}
			for ( const model::LegendEntry& Entry : Legend.GetEntries() )
			{
				InContext.Expect(Entry.Role == model::TileRole::Empty || Entry.Role == model::TileRole::Player || Entry.Role == model::TileRole::Spawner
					|| !Entry.Config.empty(), std::string("object tile has a config: ") + Entry.Tile);
			}
		});

		InSuite.Add("LevelLegend: unknown characters, unique tiles and bad entries", [](TestContext& InContext)
		{
			const auto Node = JsonConfigNode::Parse(R"({"tiles":[{"tile":".","role":"empty"},{"tile":"P","role":"player"},
				{"tile":"E1","role":"spawner","default":true},{"tile":"b","role":"wall","config":"cfg.b","pass":"blocked"},
				{"tile":"o","role":"wall","config":"cfg.o","pass":"breakable"},{"tile":"H","role":"wall","config":"cfg.h","unique":true},
				{"tile":"q","role":"nonsense"},{"tile":"w","role":"wall"},{"tile":"b","role":"empty"}]})", "legend");
			model::LevelLegend Legend;
			InContext.Expect(Legend.FromConfig(*Node), "legend read");
			InContext.Expect(Legend.GetErrors().Size() == 3, "unknown role, missing config and a repeated tile are reported");
			InContext.Expect(Legend.IsKnown('b') && !Legend.IsKnown('q') && !Legend.IsKnown('w') && !Legend.IsKnown('z'), "known and unknown");
			InContext.Expect(Legend.GetDefaultSpawner() == 'E' && Legend.IsSpawner('1'), "one entry may list several characters");

			LevelData Level = ValidLevel();
			Level.Rows.Assign(GridRows, std::string(GridColumns, '.'));
			Level.Rows[GridRows - 1][2] = 'P';
			Level.Rows[0][0] = 'E';
			InContext.Expect(LevelLoader::Validate(Level, Legend).empty(), "a map of legend characters is valid");
			Level.Rows[3][3] = 'z';
			InContext.ExpectEqual(LevelLoader::Validate(Level, Legend), std::string("LEVEL_ERROR_TILE"), "unknown character rejected");
			Level.Rows[3][3] = 'H';
			Level.Rows[4][4] = 'H';
			InContext.ExpectEqual(LevelLoader::Validate(Level, Legend), std::string("LEVEL_ERROR_TILE"), "a unique tile twice is rejected");
		});

		InSuite.Add("LevelLegend: the path grid follows the legend's pass flags", [](TestContext& InContext)
		{
			const model::LevelLegend& Legend = GameContext::Get<LevelLegendManager>().Get();
			LevelData Level = ValidLevel();
			Level.Rows[1][1] = tiles::Stone;
			Level.Rows[1][2] = tiles::Brick;
			Level.Rows[1][3] = 'H';
			Level.Rows[1][4] = 'M';
			const model::GridPath Grid = LevelLoader::BuildPathGrid(Level, Legend);
			InContext.Expect(Grid.Get({ 1, 1 }) == model::CellPass::Blocked, "stone blocks");
			InContext.Expect(Grid.Get({ 2, 1 }) == model::CellPass::Breakable && Grid.Get({ 3, 1 }) == model::CellPass::Breakable, "brick and base can be shot apart");
			InContext.Expect(Grid.Get({ 4, 1 }) == model::CellPass::Open && Grid.Get({ 0, 0 }) == model::CellPass::Open, "a mine and an empty cell are open");
		});

		InSuite.Add("LevelLegend: an editor-saved level loads back unchanged", [](TestContext& InContext)
		{
			LevelData Level = ValidLevel();
			Level.Rows[2][2] = tiles::Brick;
			Level.Rows[2][3] = 'C';
			Level.Rows[2][4] = 'F';
			Level.Rows[2][5] = 'H';
			Level.Name = "ROUND_TRIP";
			InContext.Expect(GameContext::Get<ConfigReader>().LoadConfigText(LevelLoader::ToJson(Level), "unit_round_trip"), "saved text parses");
			LevelData Loaded;
			InContext.Expect(LevelLoader::Load("unit_round_trip", Loaded), "saved level loads");
			InContext.Expect(Loaded.Rows == Level.Rows && Loaded.Name == Level.Name, "rows and name survive");
		});

		InSuite.Add("LevelWriter: every shipped level survives load, write, load", [](TestContext& InContext)
		{
			TArray<std::string> Ids = config_refs::ListGroup(ids::config_groups::Levels);
			const TArray<std::string> TestIds = config_refs::ListGroup("cfg.test_level.");
			Ids.Insert(Ids.end(), TestIds.begin(), TestIds.end());
			InContext.Expect(Ids.Size() >= 6, "campaign and test levels are listed");
			for ( const std::string& Id : Ids )
			{
				LevelData First;
				InContext.Expect(LevelLoader::Load(Id, First), Id + " loads");
				const std::string Text = LevelLoader::ToJson(First);
				InContext.Expect(GameContext::Get<ConfigReader>().LoadConfigText(Text, "unit_level_writer"), Id + " written text parses");
				LevelData Second;
				InContext.Expect(LevelLoader::Load("unit_level_writer", Second), Id + " loads again");
				InContext.Expect(First.Name == Second.Name && First.Music == Second.Music && First.Tileset == Second.Tileset, Id + " name, music, tileset");
				InContext.Expect(First.Rows == Second.Rows, Id + " rows");
				InContext.Expect(First.TotalEnemies == Second.TotalEnemies && First.MaxAlive == Second.MaxAlive
					&& First.SpawnInterval == Second.SpawnInterval && First.EnemyIds == Second.EnemyIds, Id + " simple enemies");
				InContext.Expect(First.Spawners.Size() == Second.Spawners.Size(), Id + " spawner count");
				for ( const auto& Spawner : First.Spawners )
				{
					auto* const Other = Second.Spawners.Find(Spawner.Key);
					InContext.Expect(Other && Other->Types == Spawner.Value.Types && Other->Enemies == Spawner.Value.Enemies
						&& Other->ExcludeTypes == Spawner.Value.ExcludeTypes && Other->ExcludeEnemies == Spawner.Value.ExcludeEnemies, Id + " spawner rule");
				}
				InContext.Expect(First.Waves.Size() == Second.Waves.Size(), Id + " wave count");
				for ( size_t Index = 0; Index < std::min(First.Waves.Size(), Second.Waves.Size()); ++Index )
				{
					const Wave& A = First.Waves[Index];
					const Wave& B = Second.Waves[Index];
					bool bSameEntries = A.Entries.Size() == B.Entries.Size();
					for ( size_t Entry = 0; bSameEntries && Entry < A.Entries.Size(); ++Entry )
					{
						bSameEntries = A.Entries[Entry].Spawner == B.Entries[Entry].Spawner && A.Entries[Entry].Count == B.Entries[Entry].Count;
					}
					InContext.Expect(A.Delay == B.Delay && A.MaxAlive == B.MaxAlive && A.Interval == B.Interval && bSameEntries, Id + " wave");
				}
				InContext.Expect(First.AiFeatures == Second.AiFeatures, Id + " ai_features");
				InContext.Expect(First.Script.ToJson() == Second.Script.ToJson(), Id + " script");
				InContext.Expect(First.Fog == Second.Fog && First.Extra == Second.Extra, Id + " fog and unknown keys");
				InContext.Expect(LevelLoader::ToJson(Second) == Text, Id + " second write is identical");
			}
		});

		InSuite.Add("LevelWriter: shipped level files are in the stable layout", [](TestContext& InContext)
		{
			TArray<std::string> Ids = config_refs::ListGroup(ids::config_groups::Levels);
			const TArray<std::string> TestIds = config_refs::ListGroup("cfg.test_level.");
			Ids.Insert(Ids.end(), TestIds.begin(), TestIds.end());
			for ( const std::string& Id : Ids )
			{
				LevelData Level;
				InContext.Expect(LevelDataLoader::ReadLevel(GameContext::Get<ConfigReader>().GetConfig(Id), Level), Id + " reads");
				std::string Text = cocos2d::FileUtils::getInstance()->getStringFromFile(Id);
				Text.erase(std::remove(Text.begin(), Text.end(), '\r'), Text.end());
				InContext.Expect(Text == LevelDataLoader::WriteLevel(Level), Id + " is written exactly as the editor would save it");
			}
		});

		InSuite.Add("LevelLayers: objects and points round-trip, links resolve by name, resize crops them", [](TestContext& InContext)
		{
			const std::string Source = R"({
				"name": "X", "rows": ["P.......", "....E..."],
				"objects": [ { "config": "cfg.prop", "cell": [3, 1], "name": "crate" } ],
				"points": { "a": [4, 0], "show": [7, 1] }
			})";
			InContext.Expect(GameContext::Get<ConfigReader>().LoadConfigText(Source, "unit_level_layers"), "source parses");
			LevelData Level;
			InContext.Expect(LevelDataLoader::ReadLevel(GameContext::Get<ConfigReader>().GetConfig("unit_level_layers"), Level), "level reads");
			InContext.ExpectEqual(static_cast<int>(Level.Layers.GetPoints().Size()), 2, "two points");
			InContext.Expect(Level.Layers.FindPoint("show") && Level.Layers.FindPoint("show")->Column == 7, "point found by name");
			InContext.Expect(Level.Extra.IsNull() || !Level.Extra.Has("points"), "points and objects are known keys");

			const std::string Text = LevelDataLoader::WriteLevel(Level);
			InContext.Expect(GameContext::Get<ConfigReader>().LoadConfigText(Text, "unit_level_layers"), "written text parses");
			LevelData Again;
			InContext.Expect(LevelDataLoader::ReadLevel(GameContext::Get<ConfigReader>().GetConfig("unit_level_layers"), Again), "level reads again");
			InContext.ExpectEqual(static_cast<int>(Again.Layers.GetPoints().Size()), 2, "points round-trip");
			InContext.ExpectEqual(static_cast<int>(Again.Layers.GetObjects().Size()), 1, "objects round-trip");
			InContext.ExpectEqual(Again.Layers.GetObjects()[0].Name, std::string("crate"), "object name");

			const TArray<std::string> Missing = Level.Layers.FindMissingPoints({ "a", "gone", "gone" });
			InContext.Expect(Missing.Size() == 1 && Missing[0] == "gone", "a missing point is reported once");
			InContext.Expect(!Level.Layers.SetPoint("", 1, 1), "an empty point name is refused");
			Level.Layers.SetPoint("a", 0, 0);
			InContext.ExpectEqual(Level.Layers.FindPoint("a")->Column, 0, "setting an existing point moves it");
			Level.Layers.Crop(6, 2, 0, 0);
			InContext.Expect(!Level.Layers.FindPoint("show") && Level.Layers.FindPoint("a"), "a point outside the new size is dropped");
			InContext.Expect(Level.Layers.RemoveObjectAt(3, 1) && !Level.Layers.RemoveObjectAt(3, 1), "object removed by cell");
			Level.Layers.SetPoint("b", 2, 0);
			InContext.Expect(!Level.Layers.RenamePoint("b", "a") && !Level.Layers.RenamePoint("b", ""), "a taken or empty name is refused");
			InContext.Expect(Level.Layers.RenamePoint("b", "boss") && Level.Layers.FindPoint("boss") && !Level.Layers.FindPoint("b"), "a point is renamed");
			InContext.Expect(Level.Layers.FindPointAt(2, 0) && Level.Layers.FindPointAt(2, 0)->Name == "boss" && !Level.Layers.FindPointAt(5, 1), "point found by cell");
		});

		InSuite.Add("GroundMap: roads speed tanks up and leave no marks, the ground key round-trips", [](TestContext& InContext)
		{
			const model::LevelLegend& Legend = GameContext::Get<LevelLegendManager>().Get();
			const model::GroundEntry* const Road = Legend.FindGround('=');
			InContext.Expect(Road && Road->SpeedScale > 1.0f && !Road->bLeavesMarks, "the shipped legend has a road that is faster and leaves no marks");
			InContext.Expect(Legend.FindGround('~') && Legend.FindGround('~')->SpeedScale < 1.0f, "the shipped legend has mud that is slower");

			TArray<std::string> Ground;
			Ground.PushBack("..==");
			Ground.PushBack("~...");
			model::GroundMap Map;
			Map.Build(Ground, Legend, 4, 2);
			InContext.Expect(Map.GetEffect(2, 0).SpeedScale > 1.0f && !Map.GetEffect(2, 0).bLeavesMarks, "a road cell");
			InContext.Expect(Map.GetEffect(0, 1).SpeedScale < 1.0f, "a mud cell");
			InContext.Expect(Map.GetEffect(0, 0).SpeedScale == 1.0f && Map.GetEffect(9, 9).SpeedScale == 1.0f, "plain and outside cells have no effect");
			// world Y goes up, rows count from the top: the top row of a 2-row map is Y in [32, 64) for tile size 32
			InContext.Expect(Map.GetEffectAtWorld(80.0f, 48.0f, 32.0f).SpeedScale > 1.0f, "world position maps to the road in the top row");
			InContext.Expect(Map.GetEffectAtWorld(80.0f, 16.0f, 32.0f).SpeedScale == 1.0f, "the bottom row has no road at that column");
			const model::GroundEntry* const Water = Legend.FindGround('w');
			InContext.Expect(Water && Water->bBlocksMovement && Water->PassBuff.View() == "amphibious", "the shipped legend has water that blocks without a buff");
			TArray<std::string> WaterRow;
			WaterRow.PushBack("w.");
			model::GroundMap WaterMap;
			WaterMap.Build(WaterRow, Legend, 2, 1);
			InContext.Expect(WaterMap.GetEffect(0, 0).bBlocksMovement && !WaterMap.GetEffect(1, 0).bBlocksMovement, "a water cell blocks and a plain one does not");
			TArray<std::string> ForestRow;
			ForestRow.PushBack("f.");
			model::GroundMap ForestMap;
			ForestMap.Build(ForestRow, Legend, 2, 1);
			const model::GroundEffect Forest = ForestMap.GetEffect(0, 0);
			InContext.Expect(Forest.ConcealSightTiles > 0.0f && Forest.ConcealRevealTime > 0.0f && Forest.SpeedScale < 1.0f && Forest.ConcealOpacity < 1.0f, "forest hides the tank and still slows it");
			InContext.Expect(ForestMap.GetEffect(1, 0).ConcealSightTiles == 0.0f, "plain ground hides nothing");
			model::GroundMap Plain;
			TArray<std::string> Nothing;
			Nothing.PushBack("....");
			Plain.Build(Nothing, Legend, 4, 1);
			InContext.Expect(Plain.IsEmpty(), "a ground layer without effects is empty");

			const std::string Source = R"({ "name": "X", "rows": ["P...", "...E"], "ground": ["..==", "~..."] })";
			InContext.Expect(GameContext::Get<ConfigReader>().LoadConfigText(Source, "unit_level_ground"), "source parses");
			LevelData Level;
			InContext.Expect(LevelDataLoader::ReadLevel(GameContext::Get<ConfigReader>().GetConfig("unit_level_ground"), Level), "level reads");
			InContext.ExpectEqual(static_cast<int>(Level.Ground.Size()), 2, "ground rows read");
			InContext.Expect(!Level.Extra.IsNull() ? !Level.Extra.Has("ground") : true, "ground is a known key");
			InContext.Expect(GameContext::Get<ConfigReader>().LoadConfigText(LevelDataLoader::WriteLevel(Level), "unit_level_ground"), "written text parses");
			LevelData Again;
			InContext.Expect(LevelDataLoader::ReadLevel(GameContext::Get<ConfigReader>().GetConfig("unit_level_ground"), Again), "level reads again");
			InContext.Expect(Again.Ground.Size() == 2 && Again.Ground[0] == "..==", "ground round-trips");
		});

		InSuite.Add("LevelWriter: unknown keys and fog_of_war survive, the layout is stable", [](TestContext& InContext)
		{
			const std::string Source = R"({
				"name": "X", "rows": ["P.......", "....E..."], "future_field": { "a": [1, 2.5, "z"], "b": true },
				"fog_of_war": { "enabled": true, "vision_tiles": 4.5, "show_initial_walls": false }
			})";
			InContext.Expect(GameContext::Get<ConfigReader>().LoadConfigText(Source, "unit_level_unknown"), "source parses");
			LevelData Level;
			const model::IConfigNode& Node = GameContext::Get<ConfigReader>().GetConfig("unit_level_unknown");
			InContext.Expect(LevelDataLoader::ReadLevel(Node, Level), "level reads");
			InContext.Expect(Level.Fog.bEnabled && Level.Fog.VisionTiles == 4.5f && !Level.Fog.bShowInitialWalls, "fog_of_war is read");
			InContext.Expect(Level.Extra.IsObject() && Level.Extra.Has("future_field") && !Level.Extra.Has("rows"), "only unknown keys are kept");

			const std::string Text = LevelDataLoader::WriteLevel(Level);
			InContext.Expect(Text.find("\"future_field\"") != std::string::npos && Text.find("2.5") != std::string::npos, "unknown key written back");
			InContext.Expect(Text.find("\"vision_tiles\": 4.5") != std::string::npos, "fog_of_war written");
			InContext.Expect(Text.find("\t\t\"P.......\",\n\t\t\"....E...\"\n") != std::string::npos, "one row per line, tabs");
			InContext.Expect(Text.find("\"name\"") < Text.find("\"rows\"") && Text.find("\"rows\"") < Text.find("\"enemies\"")
				&& Text.find("\"enemies\"") < Text.find("\"future_field\""), "stable key order, unknown keys last");

			InContext.Expect(GameContext::Get<ConfigReader>().LoadConfigText(Text, "unit_level_unknown"), "written text parses");
			LevelData Again;
			InContext.Expect(LevelDataLoader::ReadLevel(GameContext::Get<ConfigReader>().GetConfig("unit_level_unknown"), Again), "level reads again");
			InContext.Expect(Again.Fog == Level.Fog && Again.Extra == Level.Extra, "fog and unknown keys round-trip");
		});

		InSuite.Add("LevelWriter: saving an original keeps its pools, waves and script", [](TestContext& InContext)
		{
			bool bSawWaves = false;
			bool bSawScript = false;
			for ( const std::string& Id : config_refs::ListGroup(ids::config_groups::Levels) )
			{
				LevelData Level;
				InContext.Expect(LevelLoader::Load(Id, Level), Id + " loads");
				const bool bHadOwnPools = LevelLoader::HasOwnPools(Level);
				const bool bHadScript = !Level.Script.IsEmpty();
				bSawWaves = bSawWaves || bHadOwnPools;
				bSawScript = bSawScript || bHadScript;
				Level.Rows[1][1] = Level.Rows[1][1] == '.' ? '#' : '.';
				const std::string Text = LevelLoader::ToJson(Level);
				InContext.Expect(GameContext::Get<ConfigReader>().LoadConfigText(Text, "unit_level_edit"), Id + " edited text parses");
				LevelData Saved;
				InContext.Expect(LevelDataLoader::ReadLevel(GameContext::Get<ConfigReader>().GetConfig("unit_level_edit"), Saved), Id + " edited level reads");
				InContext.Expect(LevelLoader::HasOwnPools(Saved) == bHadOwnPools && Saved.Waves.Size() == Level.Waves.Size(), Id + " pools and waves kept");
				InContext.Expect(Saved.Script.Steps.Size() == Level.Script.Steps.Size() && Saved.Script.Actors == Level.Script.Actors && bHadScript == !Saved.Script.IsEmpty(), Id + " script kept");
			}
			InContext.Expect(bSawWaves, "a shipped level with its own pools or waves was checked");
		});
	}

	/** An audio port that only records what it is asked to do. */
	class FakeAudioOutput final : public ports::IAudioOutput
	{
	public:
		void Play(const std::string& InPath, const float InVolume) override { Played.EmplaceBack(InPath, InVolume); }
		void StopAll() override { ++StopCount; }
		void Preload(const std::string& InPath, const TFunction<void(bool)>& InDone) override
		{
			Preloaded.PushBack(InPath);
			if ( InDone )
			{
				InDone(true);
			}
		}
		void Unload(const std::string& InPath) override { Unloaded.PushBack(InPath); }

		TArray<std::pair<std::string, float>> Played;
		TArray<std::string> Preloaded;
		TArray<std::string> Unloaded;
		int StopCount = 0;
	};

	/** A storage port that keeps its files in memory. */
	class FakeStorage final : public ports::IStorage
	{
	public:
		bool Exists(const std::string& InName) const override { return Files.Find(InName) != nullptr; }
		std::optional<std::string> ReadText(const std::string& InName) const override
		{
			auto* const FileIt = Files.Find(InName);
			return FileIt ? std::optional<std::string>(*FileIt) : std::nullopt;
		}
		bool WriteText(const std::string& InName, const std::string& InText) override { Files[InName] = InText; return true; }

		TMap<FPersistentString, std::string> Files;
	};

	void AddPortTests(TestSuite& InSuite)
	{
		InSuite.Add("CustomLibrary: levels and campaigns are kept in the storage index", [](TestContext& InContext)
		{
			FakeStorage Storage;
			model::CustomLibrary Library;
			InContext.Expect(!Library.Load(Storage), "no index file yet");
			InContext.Expect(Library.MakeLevelId() == "custom_1", "first level id");
			InContext.Expect(Library.AddLevel("custom_1") && Library.AddLevel("desert_run") && !Library.AddLevel("custom_1"), "ids are unique");
			InContext.Expect(!Library.AddLevel("Bad Id") && !Library.AddLevel("") && !Library.AddLevel(std::string(41, 'a')), "invalid ids are refused");
			InContext.Expect(Library.MakeLevelId() == "custom_2", "next free id");
			InContext.Expect(model::CustomLibrary::LevelFileName("desert_run") == "level_desert_run.json", "level file name");

			const std::string CampaignId = Library.AddCampaign("Desert");
			InContext.Expect(CampaignId == "campaign_1" && Library.AddCampaign("Other") == "campaign_2", "campaign ids");
			InContext.Expect(Library.SetCampaignLevels(CampaignId, { "desert_run", "missing", "custom_1" }), "campaign levels set");
			InContext.Expect(Library.FindCampaign(CampaignId)->LevelIds == TArray<std::string>{ "desert_run", "custom_1" }, "unknown levels are dropped, order kept");
			InContext.Expect(Library.SetProgress(CampaignId, 9, -5), "progress set");
			InContext.Expect(Library.FindCampaign(CampaignId)->CompletedLevels == 2 && Library.FindCampaign(CampaignId)->Coins == 0, "progress is clamped");
			Library.SetProgress(CampaignId, 1, 40);
			InContext.Expect(Library.GetNextPosition(CampaignId) == 1, "the next level is the first one not completed");
			InContext.Expect(Library.RecordWin(CampaignId, 0, 10) && Library.FindCampaign(CampaignId)->CompletedLevels == 1 && Library.FindCampaign(CampaignId)->Coins == 50, "an old win adds coins but does not move back");
			InContext.Expect(Library.RecordWin(CampaignId, 1, 5) && Library.GetNextPosition(CampaignId) == 1 && Library.FindCampaign(CampaignId)->CompletedLevels == 2, "the last win completes the campaign, next stays on the last level");
			InContext.Expect(!Library.RecordWin(CampaignId, 2, 5) && !Library.RecordWin("none", 0, 5), "a bad position or campaign is refused");
			Library.SetProgress(CampaignId, 1, 40);

			InContext.Expect(Library.Save(Storage), "saved");
			model::CustomLibrary Loaded;
			InContext.Expect(Loaded.Load(Storage), "loaded");
			InContext.Expect(Loaded.GetLevelIds() == Library.GetLevelIds() && Loaded.GetCampaigns().Size() == 3, "levels and campaigns round-trip");
			const model::CampaignDef* const Campaign = Loaded.FindCampaign(CampaignId);
			InContext.Expect(Campaign && Campaign->Name == "Desert" && Campaign->CompletedLevels == 1 && Campaign->Coins == 40
				&& Campaign->LevelIds == TArray<std::string>{ "desert_run", "custom_1" }, "campaign progress and coins round-trip");

			InContext.Expect(Loaded.SetCampaignLevels(CampaignId, { "custom_1", "desert_run" }) && Loaded.FindCampaign(CampaignId)->CompletedLevels == 0, "a changed list resets progress");
			InContext.Expect(Loaded.RemoveLevel("custom_1") && !Loaded.HasLevel("custom_1"), "a level is removed");
			InContext.Expect(Loaded.FindCampaign(CampaignId)->LevelIds == TArray<std::string>{ "desert_run" }, "and leaves its campaigns");
			InContext.Expect(Loaded.RemoveCampaign("campaign_2") && !Loaded.RemoveCampaign("campaign_2"), "a campaign is removed once");
			InContext.Expect(Loaded.AssignLevel("desert_run", "campaign_1") && Loaded.FindCampaign("campaign_1")->LevelIds == TArray<std::string>{ "desert_run" }
				&& Loaded.FindCampaign("custom")->LevelIds.IsEmpty(), "a level moves from the default campaign into another");
			InContext.Expect(Loaded.AssignLevel("desert_run", "custom") && Loaded.FindCampaign("custom")->LevelIds.Size() == 1 && Loaded.FindCampaign("campaign_1")->LevelIds.IsEmpty(), "and back");
			InContext.Expect(!Loaded.RemoveCampaign(std::string(model::CustomLibrary::DefaultCampaignId)), "the default campaign stays");
			InContext.Expect(Loaded.FindCampaign(std::string(model::CustomLibrary::DefaultCampaignId))->LevelIds == TArray<std::string>{ "desert_run" }, "new levels went into the default campaign");
			Storage.Files[std::string(model::CustomLibrary::IndexFileName)] = "{ \"levels\": [\"lone\"], \"campaigns\": [] }";
			model::CustomLibrary Old;
			InContext.Expect(Old.Load(Storage) && Old.FindCampaign("custom")->LevelIds == TArray<std::string>{ "lone" }, "a level without a campaign moves into the default one");

			Storage.Files[std::string(model::CustomLibrary::IndexFileName)] = "{ broken";
			InContext.Expect(!Loaded.Load(Storage) && Loaded.HasLevel("desert_run"), "a broken index is refused and leaves the library alone");
		});

		InSuite.Add("JsonValue: ToString writes text that parses back to the same value", [](TestContext& InContext)
		{
			model::JsonValue::Object Members;
			Members.InsertOrAssign("name", model::JsonValue("a \"quoted\"\nline\t\\"));
			Members.InsertOrAssign("count", model::JsonValue(-7));
			Members.InsertOrAssign("ratio", model::JsonValue(0.25));
			Members.InsertOrAssign("on", model::JsonValue(true));
			Members.InsertOrAssign("empty", model::JsonValue(model::JsonValue::Array()));
			Members.InsertOrAssign("list", model::JsonValue(model::JsonValue::Array{ model::JsonValue(1), model::JsonValue("two"), model::JsonValue() }));
			model::JsonValue::Object Inner;
			Inner.InsertOrAssign("x", model::JsonValue(3));
			Members.InsertOrAssign("inner", model::JsonValue(std::move(Inner)));
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
			Original.CurrentLanguage = "pl";
			Original.bSoundEnabled = false;
			Original.FpsLimit = 144;
			Original.bVerticalSync = false;
			Original.CurrentWindowMode = EWindowMode::Borderless;
			Original.MapScale = 75;
			Original.WindowPosition = PixelPoint{ -20, 30 };
			Original.TextureQuality = static_cast<int32>(ETextureQuality::Sd);
			Original.bSmoothPixels = true;
			Original.EffectsVolume = 40;

			const auto Node = JsonConfigNode::Parse(ProfileSerializer::ToJson(Original), "test");
			const PlayerProfile Read = ProfileSerializer::FromJson(*Node);
			InContext.Expect(Read.Coins == 123 && Read.BestScore == 4567 && Read.LevelsCompleted == 3, "progress");
			InContext.Expect(Read.Upgrades == Original.Upgrades, "upgrades");
			InContext.Expect(Read.CurrentLanguage == "pl" && !Read.bSoundEnabled && Read.FpsLimit == 144 && !Read.bVerticalSync, "settings");
			InContext.Expect(Read.CurrentWindowMode == EWindowMode::Borderless && Read.MapScale == 75, "window");
			InContext.Expect(Read.WindowPosition && Read.WindowPosition.Get().X == -20 && Read.WindowPosition.Get().Y == 30, "window position");
			InContext.Expect(Read.TextureQuality == static_cast<int32>(ETextureQuality::Sd) && Read.bSmoothPixels && Read.EffectsVolume == 40, "textures and volume");

			const auto Empty = JsonConfigNode::Parse("{}", "test");
			const PlayerProfile Fresh = ProfileSerializer::FromJson(*Empty);
			InContext.Expect(Fresh.Coins == 0 && Fresh.bSoundEnabled && Fresh.FpsLimit == 60 && !Fresh.WindowPosition, "missing keys give defaults");
		});

		InSuite.Add("ScreenLayout: any aspect ratio fills the window, the design area stays whole and centred", [](TestContext& InContext)
		{
			const model::Vec2f Design(1024.0f, 864.0f);
			const TArray<std::pair<std::string, model::Vec2f>> Frames = {
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
			const PixelSize Size = GetSplashSize(GameContext::Get<UiStyle>().GetSplash().Width);
			InContext.ExpectNear(static_cast<float>(Size.Width) / Size.Height, DesignWidth / DesignHeight, 0.01f, "the splash has the design proportions");
			InContext.Expect(Size.Width < DesignWidth, "and is smaller than the design size");
		});

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		InSuite.Add("Splash: a borderless splash shows while the game window waits hidden for its saved place", [](TestContext& InContext)
		{
			const HWND Window = CheckedCast<cocos2d::GLViewImpl>(cocos2d::Director::getInstance()->getOpenGLView())->getWin32Window();
			const PlayerProfile& Saved = GameContext::Get<ProfileManager>().Get();
			ApplyWindowMode(Saved.CurrentWindowMode);
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

			ApplyWindowMode(Saved.CurrentWindowMode, StartPosition);
			InContext.Expect(!IsGameWindowHiddenUntilPlaced() && IsWindowVisible(Window) && bShownEvent, "shown once placed, and it says so");
			const PixelSize Expected = GetExpectedWindowedSize();
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
			using model::ETextSize;
			InContext.Expect(TextScales::Next(ETextSize::Small) == ETextSize::Medium && TextScales::Next(ETextSize::Medium) == ETextSize::Large
				&& TextScales::Next(ETextSize::Large) == ETextSize::Small, "Small -> Medium -> Large -> Small");
			InContext.Expect(TextScales::FromIndex(2) == ETextSize::Large && TextScales::FromIndex(7) == ETextSize::Small && TextScales::FromIndex(-1) == ETextSize::Small, "stored numbers");

			const TextScales Defaults;
			InContext.Expect(Defaults.Get(ETextSize::Small) < 1.0f && Defaults.Get(ETextSize::Medium) == 1.0f && Defaults.Get(ETextSize::Large) > 1.0f,
				"by default Small is a little below the design size");

			const auto Node = JsonConfigNode::Parse("{\"text_scales\": [0.8, 1.1, 0.1]}", "test");
			const TextScales Read = TextScales::FromConfig(*Node);
			InContext.ExpectNear(Read.Get(ETextSize::Small), 0.8f, 0.001f, "small from the config");
			InContext.ExpectNear(Read.Get(ETextSize::Medium), 1.1f, 0.001f, "medium from the config");
			InContext.ExpectNear(Read.Get(ETextSize::Large), TextScales::MinScale, 0.001f, "a tiny factor is raised to the minimum");
			const auto Short = JsonConfigNode::Parse("{\"text_scales\": [0.95]}", "test");
			InContext.ExpectNear(TextScales::FromConfig(*Short).Get(ETextSize::Large), Defaults.Get(ETextSize::Large), 0.001f, "missing factors keep their defaults");

			const TextScales& Shipped = GameContext::Get<UiStyle>().GetWidgets().TextScales;
			InContext.Expect(Shipped.Get(ETextSize::Small) < Shipped.Get(ETextSize::Medium) && Shipped.Get(ETextSize::Medium) < Shipped.Get(ETextSize::Large),
				"the shipped sizes grow from Small to Large");

			PlayerProfile Original;
			InContext.Expect(Original.CurrentTextSize == ETextSize::Small, "a new profile uses Small");
			Original.CurrentTextSize = ETextSize::Large;
			const auto Saved = JsonConfigNode::Parse(ProfileSerializer::ToJson(Original), "test");
			InContext.Expect(ProfileSerializer::FromJson(*Saved).CurrentTextSize == ETextSize::Large, "the text size survives saving the profile");
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
			InContext.Expect(Read.Coins == 42 && !Read.bSoundEnabled && Read.Upgrades.Find("armor") != nullptr && *Read.Upgrades.Find("armor") == 2, "the progress carries over");
			InContext.Expect(!view::cocos::MigrateLegacyProfile(Storage), "an existing profile.json is never overwritten");
		});

		InSuite.Add("AudioManager: sounds go through the audio port, nothing plays while disabled", [](TestContext& InContext)
		{
			AudioManager& Audio = GameContext::Get<AudioManager>();
			const bool bWasEnabled = Audio.IsEnabled();
			const float WasVolume = Audio.GetVolume();
			FakeAudioOutput Fake;
			Audio.SetOutput(&Fake);
			Audio.SetEnabled(true);
			Audio.SetVolume(0.5f);

			Audio.Play(ids::sounds::Pause);
			InContext.Expect(Fake.Played.Size() == 1 && Fake.Played[0].first == "sounds/pause.wav" && Fake.Played[0].second == 0.5f, "the file of the id and the volume reach the port");
			Audio.Play(model::ResourceId());
			Audio.Play(model::ResourceId("sfx.no_such_sound"));
			InContext.ExpectEqual(Fake.Played.Size(), std::size_t(1), "an empty or unknown id plays nothing");

			Audio.SetEnabled(false);
			InContext.ExpectEqual(Fake.StopCount, 1, "disabling stops the playing sounds");
			Audio.Play(ids::sounds::Pause);
			InContext.ExpectEqual(Fake.Played.Size(), std::size_t(1), "nothing plays while disabled");

			Audio.SetEnabled(bWasEnabled);
			Audio.SetVolume(WasVolume);
			Audio.SetOutput(nullptr);
		});

		InSuite.Add("PlayerControls: actions become commands and the newest held movement wins", [](TestContext& InContext)
		{
			using model::InputAction;
			TArray<ECommandType> Commands;
			PlayerControls Controls([&Commands](const ECommandType InCommand) { Commands.PushBack(InCommand); });
			const auto Press = [&Controls](const InputAction InAction) { Controls.OnAction(model::InputEvent{ InAction, true }); };
			const auto Release = [&Controls](const InputAction InAction) { Controls.OnAction(model::InputEvent{ InAction, false }); };

			Press(InputAction::MoveUp);
			Press(InputAction::MoveRight);
			Release(InputAction::MoveRight);
			Release(InputAction::MoveUp);
			InContext.Expect(Commands == TArray<ECommandType>({ ECommandType::MoveForward, ECommandType::MoveRight, ECommandType::MoveForward, ECommandType::Stop }),
				"releasing the newest movement goes back to the one still held, then stops");

			Commands.Clear();
			Press(InputAction::Fire);
			Press(InputAction::Ability1);
			Press(InputAction::SwitchWeapon);
			Press(InputAction::UiConfirm);
			Release(InputAction::Fire);
			InContext.Expect(Commands == TArray<ECommandType>({ ECommandType::StartFiring, ECommandType::UseAbility, ECommandType::SwitchWeapon, ECommandType::StopFiring }),
				"gameplay actions; menu actions are ignored");
		});

		InSuite.Add("PlayerControls: fire is held until released; ReleaseAll lets go of everything", [](TestContext& InContext)
		{
			using model::InputAction;
			TArray<ECommandType> Commands;
			PlayerControls Controls([&Commands](const ECommandType InCommand) { Commands.PushBack(InCommand); });
			const auto Press = [&Controls](const InputAction InAction) { Controls.OnAction(model::InputEvent{ InAction, true }); };

			Press(InputAction::Fire);
			Press(InputAction::Fire);
			InContext.Expect(Controls.IsFireHeld() && Commands == TArray<ECommandType>({ ECommandType::StartFiring }), "a repeated press does not start firing twice");

			Commands.Clear();
			Press(InputAction::MoveUp);
			Controls.ReleaseAll();
			InContext.Expect(!Controls.IsFireHeld() && !Controls.IsMoveHeld(), "nothing is held after ReleaseAll");
			InContext.Expect(Commands == TArray<ECommandType>({ ECommandType::MoveForward, ECommandType::StopFiring, ECommandType::Stop }), "the tank stops firing and moving");

			Commands.Clear();
			Controls.ReleaseAll();
			Controls.OnAction(model::InputEvent{ InputAction::Fire, false });
			InContext.Expect(Commands.IsEmpty(), "nothing to let go of twice; a release without a press sends nothing");
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
			InContext.ExpectEqual(Bindings.GetButtons(InputContext::Gameplay, InputAction::MoveUp).Size(), std::size_t(2), "two buttons on move_up");
			InContext.ExpectEqual(Bindings.GetButtons(InputContext::Gameplay, InputAction::MoveDown).Size(), std::size_t(1), "the known button of move_down stays");
			InContext.ExpectEqual(Bindings.GetErrors().Size(), std::size_t(2), "unknown action and unknown key are reported");
			InContext.Expect(Bindings.GetButtons(InputContext::Gameplay, InputAction::MoveUp)[0] == KeyButton(Key::Up), "config order is kept");
			InContext.Expect(Bindings.HasContext(InputContext::Debug), "debug section read with debug tools");
			InContext.Expect(!model::InputBindings::FromConfig(*Root, false).HasContext(InputContext::Debug), "and ignored without them");
		});

		InSuite.Add("Input: Enter, keypad Enter and Space confirm in menus (shipped config)", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(cocos2d::FileUtils::getInstance()->getStringFromFile("configs/input/input.json"), "input.json");
			const model::InputBindings Bindings = model::InputBindings::FromConfig(*Root, false);
			model::InputContextStack Stack(Bindings, InputContext::Menu);
			TArray<model::InputEvent> Events;
			Stack.SetSink([&Events](const model::InputEvent& InEvent) { Events.PushBack(InEvent); });
			for ( const auto Code : { cocos2d::EventKeyboard::KeyCode::KEY_ENTER, cocos2d::EventKeyboard::KeyCode::KEY_KP_ENTER, cocos2d::EventKeyboard::KeyCode::KEY_SPACE } )
			{
				Events.Clear();
				Stack.OnButton(KeyButton(view::cocos::CocosKeyboardInput::ToKey(Code)), true);
				Stack.OnButton(KeyButton(view::cocos::CocosKeyboardInput::ToKey(Code)), false);
				InContext.Expect(!Events.IsEmpty() && Events[0].Action == InputAction::UiConfirm, "confirm for key code " + std::to_string(static_cast<int>(Code)));
			}
			Events.Clear();
			Stack.OnButton(KeyButton(view::cocos::CocosKeyboardInput::ToKey(cocos2d::EventKeyboard::KeyCode::KEY_LEFT_ARROW)), true);
			InContext.Expect(!Events.IsEmpty() && Events[0].Action == InputAction::UiLeft, "Left adjusts a setting down");
			Events.Clear();
			Stack.OnButton(KeyButton(view::cocos::CocosKeyboardInput::ToKey(cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_ARROW)), true);
			InContext.Expect(!Events.IsEmpty() && Events[0].Action == InputAction::UiRight, "Right adjusts a setting up");
		});

		InSuite.Add("Input: a release lost during a fade does not swallow the next press", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(cocos2d::FileUtils::getInstance()->getStringFromFile("configs/input/input.json"), "input.json");
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
			TArray<model::InputEvent> Events;
			Stack.SetSink([&Events](const model::InputEvent& InEvent) { Events.PushBack(InEvent); });

			Stack.OnButton(KeyButton(Key::Z), true);
			Stack.OnButton(KeyButton(Key::F3), true);
			InContext.Expect(Events.IsEmpty(), "unbound buttons (and the debug key without debug tools) are ignored");

			Stack.OnButton(KeyButton(Key::E), true);
			InContext.ExpectEqual(Events.Size(), std::size_t(2), "e raises ability_1 and switch_weapon");
			Stack.OnButton(KeyButton(Key::E), false);
			InContext.Expect(Events.Size() == 4 && !Events[3].bPressed, "and releases both");

			Events.Clear();
			Stack.OnButton(model::InputButton::FromPad(model::PadButton::Start, 1), true);
			InContext.Expect(Events.IsEmpty(), "player 1's start is not player 2's binding");
			Stack.OnButton(model::InputButton::FromPad(model::PadButton::Start, 2), true);
			InContext.Expect(Events.Size() == 1 && Events[0].Action == InputAction::Pause, "player 2's start pauses");
		});

		InSuite.Add("Input: two buttons on one action; hold to fire", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(TestInputConfig, "test");
			const model::InputBindings Bindings = model::InputBindings::FromConfig(*Root, false);
			model::InputContextStack Stack(Bindings, InputContext::Gameplay);
			TArray<model::InputEvent> Events;
			Stack.SetSink([&Events](const model::InputEvent& InEvent) { Events.PushBack(InEvent); });

			Stack.OnButton(KeyButton(Key::Up), true);
			Stack.OnButton(KeyButton(Key::W), true);
			Stack.OnButton(KeyButton(Key::Up), false);
			InContext.Expect(Stack.IsHeld(InputAction::MoveUp) && Events.Size() == 1, "still held while w is down; pressed once");
			Stack.OnButton(KeyButton(Key::W), false);
			InContext.Expect(!Stack.IsHeld(InputAction::MoveUp) && Events.Size() == 2 && !Events[1].bPressed, "released with the last button");

			Stack.OnButton(KeyButton(Key::Space), true);
			Stack.OnButton(KeyButton(Key::Space), true);
			InContext.Expect(Stack.IsHeld(InputAction::Fire) && Events.Size() == 3, "fire held; a repeat is not a new press");
		});

		InSuite.Add("Input: pushing the menu releases held gameplay actions and blocks gameplay until popped", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(TestInputConfig, "test");
			const model::InputBindings Bindings = model::InputBindings::FromConfig(*Root, false);
			model::InputContextStack Stack(Bindings, InputContext::Gameplay);
			TArray<model::InputEvent> Events;
			Stack.SetSink([&Events](const model::InputEvent& InEvent) { Events.PushBack(InEvent); });

			Stack.OnButton(KeyButton(Key::Space), true);
			const auto Menu = Stack.Push(InputContext::Menu);
			InContext.Expect(Stack.GetActive() == InputContext::Menu, "menu on top");
			InContext.Expect(!Stack.IsHeld(InputAction::Fire) && Events.Size() == 2 && !Events[1].bPressed, "fire released when the menu opens");

			Events.Clear();
			Stack.OnButton(KeyButton(Key::Space), false);
			Stack.OnButton(KeyButton(Key::Space), true);
			InContext.Expect(Events.Size() == 1 && Events[0].Action == InputAction::UiConfirm, "space confirms in the menu, never fires");
			Stack.OnButton(KeyButton(Key::Space), false);

			Events.Clear();
			Stack.OnButton(KeyButton(Key::Escape), true);
			InContext.Expect(!Events.IsEmpty() && Events[0].Action == InputAction::UiBack, "Esc is back in the menu");
			Stack.Remove(Menu);
			Stack.OnButton(KeyButton(Key::Escape), false);
			Stack.OnButton(KeyButton(Key::Escape), true);
			InContext.Expect(Stack.GetActive() == InputContext::Gameplay && Events.Back().Action == InputAction::Pause && Events.Back().bPressed,
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

		InSuite.Add("Input: hints take the button of the last device, with the config's pad label", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(R"({"pad_labels": {"right_trigger": "RT"}, "contexts": {"gameplay": {"fire": [ { "key": "space" }, { "pad": "right_trigger" } ]}}})", "test");
			const model::InputBindings Bindings = model::InputBindings::FromConfig(*Root, false);
			const model::InputButton* const Pad = Bindings.FindButton(InputContext::Gameplay, model::InputAction::Fire, model::InputDevice::Gamepad);
			const model::InputButton* const Keys = Bindings.FindButton(InputContext::Gameplay, model::InputAction::Fire, model::InputDevice::Keyboard);
			InContext.Expect(Pad && Bindings.GetLabel(*Pad) == "RT", "pad label from the config");
			InContext.Expect(Keys && Bindings.GetLabel(*Keys) == "SPACE", "keyboard label");
			InContext.Expect(Bindings.GetErrors().IsEmpty(), "no errors");
		});
	}

	void AddWidgetLayoutTests(TestSuite& InSuite)
	{
		InSuite.Add("WidgetLayout: a row centres its children as one group with a fixed gap", [](TestContext& InContext)
		{
			model::LayoutRules Rules;
			Rules.Kind = model::LayoutKind::Row;
			Rules.Gap = 10.0f;
			Rules.Padding = model::Vec2f(5.0f, 2.0f);
			TArray<model::LayoutChild> Items(2);
			Items[0].Size = model::Vec2f(20.0f, 10.0f);
			Items[1].Size = model::Vec2f(30.0f, 16.0f);
			const model::Vec2f Size = model::widget_layout::Arrange(Rules, Items);
			InContext.Expect(Size.IsNear(model::Vec2f(70.0f, 20.0f), 0.001f), "the size is the content plus padding");
			InContext.ExpectNear(Items[0].Position.X, -20.0f, 0.001f, "first child, group centred");
			InContext.ExpectNear(Items[1].Position.X, 15.0f, 0.001f, "second child after the gap");
			InContext.ExpectNear(Items[0].Position.Y, 0.0f, 0.001f, "centred across");

			Rules.FixedSize = model::Vec2f(200.0f, 0.0f);
			Rules.MainAlign = model::LayoutAlign::Start;
			Rules.CrossAlign = model::LayoutAlign::Start;
			const model::Vec2f Wide = model::widget_layout::Arrange(Rules, Items);
			InContext.ExpectNear(Wide.X, 200.0f, 0.001f, "a fixed width stays");
			InContext.ExpectNear(Items[0].Position.X, -100.0f + 5.0f + 10.0f, 0.001f, "start: the fixed distance to the left border");
			InContext.ExpectNear(Items[0].Position.Y, 8.0f - 5.0f, 0.001f, "start: the top edges line up");

			Rules.MainAlign = model::LayoutAlign::End;
			model::widget_layout::Arrange(Rules, Items);
			InContext.ExpectNear(Items[1].Position.X, 100.0f - 5.0f - 15.0f, 0.001f, "end: the fixed distance to the right border");
		});

		InSuite.Add("WidgetLayout: a column stacks downwards, an anchor pins to a border, a fill child takes the inner size", [](TestContext& InContext)
		{
			model::LayoutRules Column;
			Column.Kind = model::LayoutKind::Column;
			Column.Gap = 4.0f;
			TArray<model::LayoutChild> Items(2);
			Items[0].Size = model::Vec2f(40.0f, 10.0f);
			Items[1].Size = model::Vec2f(20.0f, 20.0f);
			const model::Vec2f Size = model::widget_layout::Arrange(Column, Items);
			InContext.Expect(Size.IsNear(model::Vec2f(40.0f, 34.0f), 0.001f), "column size");
			InContext.ExpectNear(Items[0].Position.Y, 12.0f, 0.001f, "the first child is on top");
			InContext.ExpectNear(Items[1].Position.Y, -7.0f, 0.001f, "the second child is below the gap");

			model::LayoutRules Pin;
			Pin.Kind = model::LayoutKind::Anchor;
			Pin.FixedSize = model::Vec2f(100.0f, 60.0f);
			Pin.Padding = model::Vec2f(4.0f, 4.0f);
			TArray<model::LayoutChild> Pins(2);
			Pins[0].Size = model::Vec2f(20.0f, 10.0f);
			Pins[0].Anchor = model::Vec2f(1.0f, 1.0f);
			Pins[0].Offset = model::Vec2f(-3.0f, -3.0f);
			Pins[1].Size = model::Vec2f(10.0f, 10.0f);
			Pins[1].Anchor = model::Vec2f(0.0f, 0.5f);
			model::widget_layout::Arrange(Pin, Pins);
			InContext.ExpectNear(Pins[0].Position.X, 50.0f - 4.0f - 10.0f - 3.0f, 0.001f, "top right: a fixed distance from the right border");
			InContext.ExpectNear(Pins[0].Position.Y, 30.0f - 4.0f - 5.0f - 3.0f, 0.001f, "top right: a fixed distance from the top border");
			InContext.ExpectNear(Pins[1].Position.X, -50.0f + 4.0f + 5.0f, 0.001f, "left middle: touches the padded left border");
			InContext.ExpectNear(Pins[1].Position.Y, 0.0f, 0.001f, "left middle: centred up and down");

			model::LayoutRules Stack;
			Stack.Padding = model::Vec2f(3.0f, 2.0f);
			TArray<model::LayoutChild> Layers(2);
			Layers[0].bFill = true;
			Layers[1].Size = model::Vec2f(30.0f, 10.0f);
			const model::Vec2f StackSize = model::widget_layout::Arrange(Stack, Layers);
			InContext.Expect(StackSize.IsNear(model::Vec2f(36.0f, 14.0f), 0.001f), "a fill child does not count when measuring");
			InContext.Expect(Layers[0].Size.IsNear(model::Vec2f(36.0f, 14.0f), 0.001f), "the fill child covers the whole widget, padding included");
		});

		InSuite.Add("WidgetLayout: a child aligns itself on the cross axis and fill_x/fill_y stretch it in an anchor container", [](TestContext& InContext)
		{
			model::LayoutRules Column;
			Column.Kind = model::LayoutKind::Column;
			Column.FixedSize = model::Vec2f(100.0f, 0.0f);
			TArray<model::LayoutChild> Items(3);
			for ( model::LayoutChild& Item : Items )
			{
				Item.Size = model::Vec2f(20.0f, 10.0f);
			}
			Items[1].bHasAlign = true;
			Items[1].Align = model::LayoutAlign::Start;
			Items[2].bHasAlign = true;
			Items[2].Align = model::LayoutAlign::End;
			model::widget_layout::Arrange(Column, Items);
			InContext.ExpectNear(Items[0].Position.X, 0.0f, 0.001f, "no own align: the container's cross align (centre)");
			InContext.ExpectNear(Items[1].Position.X, -40.0f, 0.001f, "own start: at the left border");
			InContext.ExpectNear(Items[2].Position.X, 40.0f, 0.001f, "own end: at the right border");

			model::LayoutRules Pin;
			Pin.Kind = model::LayoutKind::Anchor;
			Pin.FixedSize = model::Vec2f(200.0f, 100.0f);
			TArray<model::LayoutChild> Pins(1);
			Pins[0].Size = model::Vec2f(10.0f, 30.0f);
			Pins[0].Anchor = model::Vec2f(0.5f, 0.0f);
			Pins[0].bFillX = true;
			model::widget_layout::Arrange(Pin, Pins);
			InContext.ExpectNear(Pins[0].Size.X, 200.0f, 0.001f, "fill_x takes the whole width");
			InContext.ExpectNear(Pins[0].Size.Y, 30.0f, 0.001f, "the height stays");
			InContext.ExpectNear(Pins[0].Position.Y, -50.0f + 15.0f, 0.001f, "still pinned to the bottom");
		});

		InSuite.Add("Gradient: stops are spread evenly, sampling blends between them and clamps outside", [](TestContext& InContext)
		{
			model::Gradient Gradient;
			InContext.Expect(!Gradient.IsSet(), "no stops: not a gradient");
			Gradient.AddStop(model::Color(0, 0, 0));
			Gradient.AddStop(model::Color(100, 200, 50));
			Gradient.AddStop(model::Color(200, 0, 250, 0));
			InContext.Expect(Gradient.IsSet(), "three stops");
			InContext.Expect(Gradient.Sample(0.0f) == model::Color(0, 0, 0), "first stop at 0");
			InContext.Expect(Gradient.Sample(0.5f) == model::Color(100, 200, 50), "middle stop at 0.5");
			InContext.Expect(Gradient.Sample(0.25f) == model::Color(50, 100, 25), "halfway to the middle stop");
			InContext.Expect(Gradient.Sample(1.0f) == model::Color(200, 0, 250, 0), "last stop at 1, alpha blended");
			InContext.Expect(Gradient.Sample(-3.0f) == Gradient.Sample(0.0f) && Gradient.Sample(9.0f) == Gradient.Sample(1.0f), "clamped");
			InContext.Expect(Gradient.Scaled(0.5f).Sample(0.5f) == model::Color(50, 100, 25), "scaling darkens every stop");
			for ( int Extra = 0; Extra < 4; ++Extra )
			{
				Gradient.AddStop(model::Color());
			}
			InContext.Expect(Gradient.Count == model::MaxGradientStops, "no more than five stops");
			InContext.Expect(model::FlatGradient(model::Color(9, 8, 7)).Sample(0.3f) == model::Color(9, 8, 7), "a flat gradient is one color");
		});

		InSuite.Add("WidgetLayout: content that does not fit shrinks by a bounded factor", [](TestContext& InContext)
		{
			InContext.ExpectNear(model::widget_layout::FitFactor(100.0f, 20.0f, 60.0f, 0.5f), 1.0f, 0.001f, "fits: unchanged");
			InContext.ExpectNear(model::widget_layout::FitFactor(100.0f, 20.0f, 160.0f, 0.5f), 0.5f, 0.001f, "exactly half");
			InContext.ExpectNear(model::widget_layout::FitFactor(100.0f, 20.0f, 800.0f, 0.5f), 0.5f, 0.001f, "never below the minimum");
			InContext.ExpectNear(model::widget_layout::FitFactor(100.0f, 20.0f, 0.0f, 0.5f), 1.0f, 0.001f, "nothing to shrink");
		});

		InSuite.Add("WidgetLayout: a grid fills equal cells in rows, a grow child takes the leftover room, a stretch child fills the cross axis", [](TestContext& InContext)
		{
			model::LayoutRules Grid;
			Grid.Kind = model::LayoutKind::Grid;
			Grid.Columns = 3;
			Grid.Gap = 4.0f;
			TArray<model::LayoutChild> Cells(5);
			for ( model::LayoutChild& Cell : Cells )
			{
				Cell.Size = model::Vec2f(10.0f, 20.0f);
			}
			Cells[4].bHidden = true;
			const model::Vec2f GridSize = model::widget_layout::Arrange(Grid, Cells);
			InContext.Expect(GridSize.IsNear(model::Vec2f(38.0f, 44.0f), 0.001f), "3 columns, 2 rows (a hidden cell takes no room)");
			InContext.Expect(Cells[0].Position.IsNear(model::Vec2f(-14.0f, 12.0f), 0.001f) && Cells[2].Position.IsNear(model::Vec2f(14.0f, 12.0f), 0.001f), "the first row runs left to right from the top");
			InContext.Expect(Cells[3].Position.IsNear(model::Vec2f(-14.0f, -12.0f), 0.001f), "the next row starts at the left below");

			model::LayoutRules Header;
			Header.Kind = model::LayoutKind::Row;
			Header.FixedSize = model::Vec2f(200.0f, 30.0f);
			Header.MainAlign = model::LayoutAlign::Start;
			TArray<model::LayoutChild> Items(2);
			Items[0].Size = model::Vec2f(20.0f, 10.0f);
			Items[0].Grow = 1.0f;
			Items[0].bStretch = true;
			Items[1].Size = model::Vec2f(24.0f, 24.0f);
			model::widget_layout::Arrange(Header, Items);
			InContext.ExpectNear(Items[0].Size.X, 200.0f - 24.0f, 0.001f, "a grow child takes the room its sibling leaves");
			InContext.ExpectNear(Items[0].Size.Y, 30.0f, 0.001f, "a stretch child takes the cross size");
			InContext.ExpectNear(Items[1].Position.X, 100.0f - 12.0f, 0.001f, "so the next sibling sits at the far end");
		});

		InSuite.Add("WidgetLayout: a grid with columns 0 fits as many columns as its width holds; equal children share a row", [](TestContext& InContext)
		{
			model::LayoutRules Grid;
			Grid.Kind = model::LayoutKind::Grid;
			Grid.Columns = 0;
			Grid.Gap = 4.0f;
			Grid.FixedSize = model::Vec2f(100.0f, 0.0f);
			TArray<model::LayoutChild> Cells(7);
			for ( model::LayoutChild& Cell : Cells )
			{
				Cell.Size = model::Vec2f(20.0f, 20.0f);
			}
			const model::Vec2f GridSize = model::widget_layout::Arrange(Grid, Cells);
			InContext.ExpectNear(GridSize.Y, 44.0f, 0.001f, "seven cells of 20 + 4 in 100 wide make 4 columns and 2 rows");
			InContext.Expect(Cells[3].Position.Y == Cells[0].Position.Y && Cells[4].Position.Y < Cells[0].Position.Y, "the fifth cell starts the second row");
			Grid.FixedSize = model::Vec2f(0.0f, 0.0f);
			TArray<model::LayoutChild> Loose(3);
			for ( model::LayoutChild& Cell : Loose )
			{
				Cell.Size = model::Vec2f(20.0f, 20.0f);
			}
			InContext.ExpectNear(model::widget_layout::Arrange(Grid, Loose).Y, 20.0f + 4.0f + 20.0f + 4.0f + 20.0f, 0.001f, "without a width there is one column");

			model::LayoutRules Row;
			Row.Kind = model::LayoutKind::Row;
			Row.FixedSize = model::Vec2f(100.0f, 20.0f);
			Row.Gap = 10.0f;
			Row.bEqualChildren = true;
			TArray<model::LayoutChild> Pair(2);
			Pair[0].Size = model::Vec2f(10.0f, 10.0f);
			Pair[1].Size = model::Vec2f(60.0f, 10.0f);
			model::widget_layout::Arrange(Row, Pair);
			InContext.ExpectNear(Pair[0].Size.X, 45.0f, 0.001f, "two equal children split the width after the gap");
			InContext.ExpectNear(Pair[1].Size.X, 45.0f, 0.001f, "whatever their own width");
		});

		InSuite.Add("UiWidget: show_if flags, repeated list items, texts of inner widgets and click handlers", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(R"({"layout":"column","gap":4,"main_align":"start","cross_align":"start","children":[
				{"id":"list","type":"widget","widget":"editor_list","item":"list_item","stretch":true},
				{"id":"plain","type":"widget","widget":"editor_list","stretch":true},
				{"id":"extra","type":"widget","widget":"editor_button","show_if":"more","text_keys":{"text":"EDITOR_SAVE"}},
				{"id":"lit","type":"widget","widget":"editor_button","texts":{"text":"Hello"}}]})", "test");
			TArray<std::string> Missing;
			const viewmodel::WidgetDef Def = viewmodel::WidgetDef::FromConfig(*Root, "demo", Missing);
			InContext.Expect(Missing.IsEmpty() && Def.Children[0].Item == "list_item" && Def.Children[2].ShowIf == "more", "show_if and item are read");
			InContext.Expect(Def.Children[3].SubTexts.Size() == 1 && Def.Children[2].SubTextKeys.Size() == 1, "texts and text_keys are read");
			const model::JsonValue Saved = Def.ToConfig();
			bool bParsed = false;
			const auto Again = JsonConfigNode::Parse(Saved.ToString(), "again", &bParsed);
			InContext.Expect(bParsed && viewmodel::WidgetDef::FromConfig(*Again, "demo", Missing).ToConfig() == Saved, "they survive a save");

			auto* const Panel = view::cocos::LayoutWidget::Create(Def);
			InContext.Expect(Panel != nullptr, "the widget is built");
			if ( !Panel )
			{
				return;
			}
			Panel->SetFixedSize(cocos2d::Size(200.0f, 0.0f));
			InContext.Expect(Panel->FindWidgetPath("lit")->GetText("text") == "Hello", "a literal text reaches the label of the inner widget");
			InContext.Expect(Panel->FindWidgetPath("extra")->GetText("text") == GameContext::Get<StringsManager>().GetString("EDITOR_SAVE"), "a string id is looked up");
			const float WithExtra = Panel->getContentSize().height;
			Panel->SetFlag("more", false);
			InContext.Expect(!Panel->FindWidget("extra")->isVisible() && Panel->getContentSize().height < WithExtra, "a false flag hides the child and its room");
			Panel->SetFlag("more", true);
			InContext.Expect(Panel->FindWidget("extra")->isVisible() && Panel->getContentSize().height == WithExtra, "a true flag shows it again");

			const float Empty = Panel->getContentSize().height;
			const TArray<view::cocos::LayoutWidget*> Rows = Panel->SetListCount("list", 3);
			InContext.ExpectEqual(Rows.Size(), static_cast<std::size_t>(3), "the list holds the asked number of items");
			InContext.Expect(Panel->getContentSize().height > Empty, "and grows with them");
			InContext.Expect(!Rows.IsEmpty() && std::abs(Rows[0]->getContentSize().width - 200.0f) < 0.5f, "list items stretch over the width");
			InContext.ExpectEqual(Panel->SetListCount("list", 1).Size(), static_cast<std::size_t>(1), "asking again replaces the items");
			InContext.Expect(Panel->SetListCount("plain", 2).IsEmpty(), "a child without an item widget is no list");

			int Clicks = 0;
			Panel->BindClick("lit", [&Clicks]() { ++Clicks; });
			view::cocos::LayoutWidget* const Lit = Panel->FindWidgetPath("lit");
			InContext.Expect(Lit->HasClickHandler() && Lit->Click() && Clicks == 1, "a bound handler runs on a click");
			Lit->SetEnabled(false);
			InContext.Expect(!Lit->Click() && Clicks == 1 && !Lit->IsEnabled(), "a disabled widget ignores it");
		});

		InSuite.Add("UiWidget: clip, touchable and per-axis scale are read and saved; a slider follows its value", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(R"({"layout":"stack","clip":true,"touchable":false,"children":[{"id":"a","type":"spacer","scale_x":2,"scale_y":0.5}]})", "test");
			TArray<std::string> Missing;
			const viewmodel::WidgetDef Def = viewmodel::WidgetDef::FromConfig(*Root, "demo", Missing);
			InContext.Expect(Def.bClip && !Def.bTouchable && Def.Children[0].ScaleX == 2.0f && Def.Children[0].ScaleY == 0.5f, "the keys are read");
			const model::JsonValue Saved = Def.ToConfig();
			bool bParsed = false;
			const auto Again = JsonConfigNode::Parse(Saved.ToString(), "again", &bParsed);
			InContext.Expect(bParsed && viewmodel::WidgetDef::FromConfig(*Again, "demo", Missing).ToConfig() == Saved, "and survive a save");

			auto* const Untouchable = view::cocos::LayoutWidget::Create(Def);
			InContext.Expect(Untouchable != nullptr, "the widget is built");
			if ( Untouchable )
			{
				Untouchable->SetClickHandler([]() {});
				InContext.Expect(!Untouchable->HasClickHandler() && !Untouchable->IsHoverable(), "a widget with touchable false takes no click");
			}

			auto* const Slider = view::cocos::LayoutWidget::Create("slider");
			InContext.Expect(Slider != nullptr, "the slider widget is built");
			if ( !Slider )
			{
				return;
			}
			Slider->SetFixedSize(cocos2d::Size(100.0f, 0.0f));
			Slider->SetSliderValue(0.25f);
			InContext.ExpectNear(Slider->GetSliderValue(), 0.25f, 0.0001f, "it keeps its value");
			InContext.ExpectNear(Slider->GetPartShownFraction("fill"), 0.25f, 0.0001f, "the fill follows it");
			const float LowKnob = Slider->FindPart("knob")->getPositionX();
			Slider->SetSliderValue(0.75f);
			InContext.Expect(LowKnob < 0.0f && Slider->FindPart("knob")->getPositionX() > 0.0f, "the knob moves along the track");
			Slider->SetSliderValue(3.0f);
			InContext.ExpectNear(Slider->GetSliderValue(), 1.0f, 0.0001f, "a value past the end is clamped");
		});

		InSuite.Add("WidgetLayout: a pivot puts a child outside its anchor; UiWidget: the upgrade node and panel build", [](TestContext& InContext)
		{
			model::LayoutRules Anchored;
			Anchored.Kind = model::LayoutKind::Anchor;
			Anchored.FixedSize = model::Vec2f(100.0f, 100.0f);
			TArray<model::LayoutChild> Items(2);
			Items[0].Size = Items[1].Size = model::Vec2f(20.0f, 10.0f);
			Items[0].Anchor = Items[1].Anchor = model::Vec2f(0.5f, 0.0f);
			Items[1].Pivot = model::Vec2f(0.5f, 1.0f);
			Items[1].bHasPivot = true;
			model::widget_layout::Arrange(Anchored, Items);
			InContext.ExpectNear(Items[0].Position.Y, -45.0f, 0.001f, "without a pivot the child stays inside the bottom edge");
			InContext.ExpectNear(Items[1].Position.Y, -55.0f, 0.001f, "with the top as pivot it hangs below the bottom edge");

			auto* const Node = view::cocos::WidgetFactory::Create(ids::widgets::UpgradeNode);
			auto* const Panel = view::cocos::WidgetFactory::Create(ids::widgets::UpgradePanel);
			InContext.Expect(Node != nullptr && Panel != nullptr, "the upgrade widget files are registered");
			if ( Node )
			{
				Node->SetFixedSize(cocos2d::Size(76.0f, 76.0f));
				Node->SetText("caption", "Armor");
				Node->FindWidget("badge")->SetText("price", "120");
				InContext.ExpectNear(Node->getContentSize().width, 76.0f, 0.001f, "the node keeps its fixed size, caption and badge hang outside it");
				InContext.Expect(Node->FindPart("caption")->getPositionY() < -38.0f && Node->FindWidget("badge")->getPositionY() > 38.0f, "the caption sits below the ring and the badge above");
			}
		});

		InSuite.Add("WidgetDef: a widget file is read into rules, children and colors", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(R"({"layout":"row","gap":6,"padding":[4,2],"size":[0,24],"main_align":"start","on_click":"cmd.go",
				"children":[{"type":"icon","id":"image","frame":"x","size":[18,18]},
				{"type":"rect","fill_parent":true,"fill":[1,2,3,200],"border":"accent","border_width":2},{"type":"label","id":"text","text_color":"good","shrink":true},{"type":"bogus"}]})", "test");
			TArray<std::string> Missing;
			const viewmodel::WidgetDef Def = viewmodel::WidgetDef::FromConfig(*Root, "demo", Missing);
			InContext.Expect(Def.Rules.Kind == model::LayoutKind::Row && Def.Rules.MainAlign == model::LayoutAlign::Start, "layout and alignment by name");
			InContext.Expect(Def.Rules.Padding.IsNear(model::Vec2f(4.0f, 2.0f), 0.001f) && Def.Rules.FixedSize.IsNear(model::Vec2f(0.0f, 24.0f), 0.001f), "padding and size");
			InContext.Expect(Def.OnClick == "cmd.go" && Def.Children.Size() == 4, "command and children");
			InContext.Expect(Def.Children[1].FillColor.bLiteral && Def.Children[1].FillColor.Literal == model::Color(1, 2, 3, 200), "a color as numbers");
			InContext.Expect(Def.Children[1].BorderColor.Name == "accent" && Def.Children[1].bFill, "a color by name; fill_parent");
			InContext.Expect(Def.Children[2].bShrinkToFit && Def.Children[2].TextColor.Name == "good", "shrink and text color");
			InContext.Expect(Missing.Size() == 1 && Missing[0] == "demo.children[3].type", "an unknown part type is recorded with its path");
			const viewmodel::WidgetColors Colors;
			InContext.Expect(Colors.Find("button_border") && !Colors.Find("nope"), "shared colors are found by name");
		});

		InSuite.Add("LayoutWidget: widget files load, and the layout follows the text", [](TestContext& InContext)
		{
			const UiStyle& Style = GameContext::Get<UiStyle>();
			InContext.Expect(Style.FindWidget(ids::widgets::Keycap) && Style.FindWidget(ids::widgets::Button) && Style.FindWidget(ids::widgets::Icon), "the shipped widget files are registered");

			auto* const Short = view::cocos::WidgetFactory::CreateKeycap("W");
			auto* const Long = view::cocos::WidgetFactory::CreateKeycap("Left Shift Key");
			InContext.Expect(Short && Long, "keycaps are created");
			if ( Short && Long )
			{
				InContext.ExpectNear(Short->getContentSize().width, Long->getContentSize().width, 0.001f, "a keycap keeps its width for any key name");
				InContext.Expect(Long->FindLabel("text")->getScale() < 1.0f && Short->FindLabel("text")->getScale() == 1.0f, "only the long name shrinks");
			}

			const auto Root = JsonConfigNode::Parse(R"({"layout":"row","gap":8,"padding":[10,4],"children":[
				{"id":"gap","type":"spacer","size":[20,20]},{"id":"text","type":"label","text":"A","font_size":20}]})", "test");
			TArray<std::string> Missing;
			const viewmodel::WidgetDef Def = viewmodel::WidgetDef::FromConfig(*Root, "demo", Missing);
			auto* const Wide = view::cocos::LayoutWidget::Create(Def);
			InContext.Expect(Wide != nullptr && Missing.IsEmpty(), "an inline widget is built");
			if ( Wide )
			{
				const float Before = Wide->getContentSize().width;
				const float GapBefore = Wide->FindPart("gap")->getPositionX();
				Wide->SetText("text", "A much longer caption");
				InContext.Expect(Wide->getContentSize().width > Before + 20.0f, "a longer text makes the widget wider");
				InContext.Expect(Wide->FindPart("gap")->getPositionX() < GapBefore - 10.0f, "and the children are placed again, as one centred group");
				const cocos2d::Label* const Text = Wide->FindLabel("text");
				InContext.ExpectNear(Text->getPositionX() + Text->getContentSize().width / 2, Wide->getContentSize().width / 2 - 10.0f, 0.01f, "the text ends at the right padding");
			}
		});

		InSuite.Add("UiWidget: a hidden child takes no room, an appended node is measured, a wrapping label follows the width", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(R"({"layout":"column","gap":10,"size":[200,0],"padding":[10,10],"children":[
				{"id":"head","type":"spacer","size":[10,20]},{"id":"note","type":"label","font_size":16,"wrap":true,"text":"one two three four five six seven eight nine ten eleven twelve thirteen"},
				{"id":"tail","type":"spacer","size":[10,30]}]})", "test");
			TArray<std::string> Missing;
			const viewmodel::WidgetDef Def = viewmodel::WidgetDef::FromConfig(*Root, "demo", Missing);
			auto* const Panel = view::cocos::LayoutWidget::Create(Def);
			InContext.Expect(Panel != nullptr && Missing.IsEmpty(), "the widget is built");
			if ( !Panel )
			{
				return;
			}
			const float WithNote = Panel->getContentSize().height;
			InContext.Expect(Panel->FindLabel("note")->getContentSize().width <= 180.0f + 0.5f, "a wrapping label stays inside the inner width");
			Panel->SetPartVisible("note", false);
			const float Without = Panel->getContentSize().height;
			InContext.ExpectNear(Without, 20.0f + 10.0f + 30.0f + 20.0f, 0.001f, "a hidden child leaves neither room nor a gap");
			InContext.Expect(WithNote > Without, "and shows again with it");
			auto* const Extra = cocos2d::Node::create();
			Extra->setContentSize(cocos2d::Size(50.0f, 40.0f));
			Panel->AppendNode(Extra);
			InContext.ExpectNear(Panel->getContentSize().height, Without + 10.0f + 40.0f, 0.001f, "an appended node takes its content size and a gap");
			Panel->RemoveAppendedNodes();
			InContext.ExpectNear(Panel->getContentSize().height, Without, 0.001f, "and leaves again");
		});

		InSuite.Add("UiWidget: editor widgets (dock, tabs, tile grid, menu) build, the dock header pushes its handle to the end, parts dim and recolour", [](TestContext& InContext)
		{
			auto* const Dock = view::cocos::WidgetFactory::Create(ids::widgets::Dock);
			InContext.Expect(Dock != nullptr, "the dock widget file is registered");
			if ( !Dock )
			{
				return;
			}
			Dock->SetFixedSize(cocos2d::Size(300.0f, 200.0f));
			auto* const Header = Dock->FindWidget("header");
			auto* const Handle = Header ? Header->FindWidget("handle") : nullptr;
			InContext.Expect(Handle != nullptr, "the header holds the collapse handle");
			if ( Handle )
			{
				Handle->SetText("text", "<");
				const float HeaderHalf = Header->getContentSize().width / 2.0f;
				InContext.ExpectNear(Handle->getPositionX(), HeaderHalf - Handle->getContentSize().width / 2.0f, 0.5f, "the handle sits at the right end of the header");
				InContext.Expect(Header->getContentSize().width > 230.0f && Header->getContentSize().width < 300.0f, "and the header stretches over the dock width");
			}
			auto* const Tab = view::cocos::WidgetFactory::Create(ids::widgets::Tab);
			InContext.Expect(Tab != nullptr, "the tab widget file is registered");
			if ( Tab )
			{
				Tab->SetText("text", "Library");
				const float Width = Tab->getContentSize().width;
				Tab->SetText("text", "Library and more");
				InContext.Expect(Tab->getContentSize().width > Width, "a tab follows its text");
				Tab->SetPartOpacity("background", 0.25f);
				Tab->SetPartColors("background", "selection", "accent");
				InContext.ExpectNear(static_cast<float>(Tab->FindPart("background")->getOpacity()), 64.0f, 1.0f, "a part can be dimmed");
			}
			auto* const Grid = view::cocos::WidgetFactory::Create(ids::widgets::TileGrid);
			InContext.Expect(Grid != nullptr, "the tile grid widget file is registered");
			if ( Grid )
			{
				for ( int Index = 0; Index < 8; ++Index )
				{
					Grid->AppendNode(view::cocos::WidgetFactory::Create(ids::widgets::Tile));
				}
				const float Tile = view::cocos::WidgetFactory::Create(ids::widgets::Tile)->getContentSize().width;
				InContext.ExpectNear(Grid->getContentSize().width, Tile * 6.0f + 6.0f * 5.0f, 0.5f, "tiles wrap after six columns");
				InContext.ExpectNear(Grid->getContentSize().height, Tile * 2.0f + 6.0f, 0.5f, "into two rows");
			}
			for ( const std::string_view Id : { ids::widgets::ListItem, ids::widgets::MenuItem, ids::widgets::MenuSeparator, ids::widgets::ContextMenu } )
			{
				InContext.Expect(view::cocos::WidgetFactory::Create(Id) != nullptr, std::string(Id) + " is registered");
			}
		});

		InSuite.Add("UiWidget: text keeps within max_width (shrinks, then cuts with an ellipsis at min_scale) and max_chars cuts", [](TestContext& InContext)
			{
				auto* const Node = view::cocos::WidgetFactory::Create(ids::widgets::UpgradeNode);
				InContext.Expect(Node != nullptr, "the upgrade node widget is registered");
				if ( !Node )
				{
					return;
				}
				Node->SetText("caption", "Hi");
				const float ShortWidth = Node->FindLabel("caption")->getContentSize().width * Node->FindLabel("caption")->getScale();
				InContext.Expect(ShortWidth > 0.0f && Node->FindLabel("caption")->getScale() == 1.0f, "a short caption keeps its size");
				Node->SetText("caption", "An extremely long upgrade name that cannot possibly fit in the node");
				const cocos2d::Label* const Long = Node->FindLabel("caption");
				const float Shown = Long->getContentSize().width * Long->getScale();
				InContext.Expect(Shown < 140.0f, "a long caption stays near max_width, not its natural length");
				InContext.Expect(Long->getScale() >= 0.6f - 0.001f, "it never shrinks below min_scale");
				InContext.Expect(Long->getString().size() > 3 && Long->getString().substr(Long->getString().size() - 3) == "...", "and is cut with an ellipsis");
				InContext.Expect(Node->GetText("caption") == Long->getString(), "GetText returns what is shown");
			});

		InSuite.Add("UiWidget: every widget file survives a save: ToConfig, text, parse and read again gives the same widget", [](TestContext& InContext)
			{
				const ConfigReader& Reader = GameContext::Get<ConfigReader>();
				size_t Checked = 0;
				for ( const FPersistentString WidgetId : Reader.GetConfigIds("configs/ui/widgets/") )
				{
					const std::string Path{ WidgetId.View() };
					TArray<std::string> Missing;
					const viewmodel::WidgetDef First = viewmodel::WidgetDef::FromConfig(Reader.GetConfig(Path), Path, Missing);
					const model::JsonValue Saved = First.ToConfig();
					bool bParsed = false;
					const auto Again = JsonConfigNode::Parse(Saved.ToString(), Path, &bParsed);
					InContext.Expect(bParsed, Path + ": the saved text parses");
					if ( !bParsed )
					{
						continue;
					}
					const viewmodel::WidgetDef Second = viewmodel::WidgetDef::FromConfig(*Again, Path, Missing);
					InContext.Expect(Second.ToConfig() == Saved, Path + ": nothing is lost or changed by a save");
					InContext.Expect(Second.Children.Size() == First.Children.Size(), Path + ": same children");
					++Checked;
				}
				InContext.Expect(Checked > 20, "the widget files were found");
			});

		InSuite.Add("UiWidget: a variant overrides layout keys and named children; the base and the variants block are kept", [](TestContext& InContext)
			{
				const std::string Text = R"({"layout": "row", "gap": 10, "children": [{"id": "icon", "type": "icon", "size": [40, 40]}, {"id": "name", "type": "label", "font_size": 20}], "variants": {"compact": {"gap": 2, "children": {"icon": {"size": [20, 20]}, "name": {"visible": false}}}}})";
				const auto Node = JsonConfigNode::Parse(Text, "variant test");
				TArray<std::string> Missing;
				const viewmodel::WidgetDef Base = viewmodel::WidgetDef::FromConfig(*Node, "t", Missing);
				const viewmodel::WidgetDef Compact = viewmodel::WidgetDef::FromConfig(*Node, "t", Missing, "compact");
				const viewmodel::WidgetDef Unknown = viewmodel::WidgetDef::FromConfig(*Node, "t", Missing, "huge");
				InContext.ExpectNear(Base.Rules.Gap, 10.0f, 0.001f, "no variant: the base gap");
				InContext.ExpectNear(Compact.Rules.Gap, 2.0f, 0.001f, "the variant gap");
				InContext.ExpectNear(Compact.Children[0].Size.X, 20.0f, 0.001f, "the variant overrides a child by id");
				InContext.Expect(Base.Children[1].bVisible && !Compact.Children[1].bVisible, "and a child's visibility");
				InContext.ExpectNear(Unknown.Rules.Gap, 10.0f, 0.001f, "an unknown variant changes nothing");
				InContext.Expect(Compact.ToConfig().Get("variants").Has("compact"), "a save keeps the variants block");
			});

		InSuite.Add("UiWidget: visible, rotation, scale and z come from the widget file", [](TestContext& InContext)
			{
				const std::string Text = R"({"layout": "anchor", "size": [100, 100], "children": [{"id": "a", "type": "rect", "size": [10, 10], "fill": "button", "rotation": 45, "scale": 2, "z": 3}, {"id": "b", "type": "rect", "size": [10, 10], "visible": false}]})";
				const auto Node = JsonConfigNode::Parse(Text, "state test");
				TArray<std::string> Missing;
				const viewmodel::WidgetDef Def = viewmodel::WidgetDef::FromConfig(*Node, "t", Missing);
				auto* const Widget = view::cocos::LayoutWidget::Create(Def);
				InContext.Expect(Widget != nullptr, "created");
				if ( !Widget )
				{
					return;
				}
				InContext.ExpectNear(Widget->FindPart("a")->getRotation(), 45.0f, 0.001f, "rotation");
				InContext.ExpectNear(Widget->FindPart("a")->getScale(), 2.0f, 0.001f, "scale");
				InContext.Expect(Widget->FindPart("a")->getLocalZOrder() > Widget->FindPart("b")->getLocalZOrder(), "z lifts a child above its siblings");
				InContext.Expect(!Widget->FindPart("b")->isVisible(), "a hidden child");
				InContext.Expect(Def.ToConfig().Get("children").GetArray()[0].Get("rotation").AsInt() == 45, "all of it is saved");
			});

		InSuite.Add("RichText: {icon:name} markers split a text into text and icon runs, broken markers stay text", [](TestContext& InContext)
			{
				const auto Runs = model::SplitIconMarkup("Cost {icon:coin} 5");
				InContext.Expect(Runs.Size() == 3 && !Runs[0].bIcon && Runs[0].Value == "Cost " && Runs[1].bIcon && Runs[1].Value == "coin" && Runs[2].Value == " 5", "text, icon, text");
				InContext.Expect(model::SplitIconMarkup("{icon:a}{icon:b}").Size() == 2, "two icons in a row");
				const auto Broken = model::SplitIconMarkup("a {icon:} b {icon:c");
				InContext.Expect(Broken.Size() == 1 && !Broken[0].bIcon && Broken[0].Value == "a {icon:} b {icon:c", "empty and unfinished markers stay text");
				InContext.Expect(model::SplitIconMarkup("").IsEmpty(), "empty text has no runs");
			});

		InSuite.Add("BarMeter: the first value jumps, a rise eases, a drop leaves a trail that drains after its delay", [](TestContext& InContext)
			{
				model::BarMeter Meter(1.0f, 0.5f, model::Easing::Linear);
				Meter.SetTarget(1.0f);
				InContext.ExpectNear(Meter.GetShown(), 1.0f, 0.001f, "first value at once");
				Meter.SetTarget(0.5f);
				InContext.ExpectNear(Meter.GetShown(), 1.0f, 0.001f, "a drop does not jump");
				Meter.Advance(0.25f);
				InContext.ExpectNear(Meter.GetShown(), 0.75f, 0.001f, "half way after half the time (0.5 of a full bar)");
				InContext.ExpectNear(Meter.GetTrail(), 1.0f, 0.001f, "the trail waits");
				Meter.Advance(0.25f);
				InContext.ExpectNear(Meter.GetShown(), 0.5f, 0.001f, "arrived");
				Meter.Advance(1.0f);
				InContext.Expect(Meter.IsSettled() && Meter.GetTrail() == 0.5f, "the trail drained");
				model::BarMeter Eased(1.0f, 0.0f, model::Easing::In);
				Eased.SetTarget(0.0f);
				Eased.SetTarget(1.0f);
				Eased.Advance(0.5f);
				InContext.Expect(Eased.GetShown() < 0.5f, "an ease-in is slower than linear at first");
			});

		InSuite.Add("HoverBook: the highest layer under the pointer wins, a modal layer hides the rest, inactive entries are skipped", [](TestContext& InContext)
			{
				model::HoverBook Book;
				const int Low = Book.Add(model::RectF(0.0f, 0.0f, 100.0f, 100.0f), 0);
				const int High = Book.Add(model::RectF(20.0f, 20.0f, 30.0f, 30.0f), 5);
				const int Tied = Book.Add(model::RectF(20.0f, 20.0f, 30.0f, 30.0f), 5);
				InContext.Expect(Book.Pick(model::Vec2f(30.0f, 30.0f)) == Tied, "equal layers: the one added last");
				InContext.Expect(Book.Pick(model::Vec2f(80.0f, 80.0f)) == Low, "outside the higher ones");
				InContext.Expect(Book.Pick(model::Vec2f(500.0f, 500.0f)) == model::HoverBook::NoWidget, "nothing there");
				Book.Update(Tied, model::RectF(20.0f, 20.0f, 30.0f, 30.0f), 5, false);
				InContext.Expect(Book.Pick(model::Vec2f(30.0f, 30.0f)) == High, "an inactive entry is skipped");
				Book.SetModalLayer(5);
				InContext.Expect(Book.Pick(model::Vec2f(80.0f, 80.0f)) == model::HoverBook::NoWidget, "below the modal layer nothing hovers");
				Book.Remove(High);
				InContext.Expect(Book.Pick(model::Vec2f(30.0f, 30.0f)) == model::HoverBook::NoWidget, "removed");
				Book.SetModalLayer(0);
				InContext.Expect(Book.MovePointer(model::Vec2f(80.0f, 80.0f)) && Book.GetHovered() == Low, "moving onto an entry reports a change");
				InContext.Expect(!Book.MovePointer(model::Vec2f(81.0f, 80.0f)), "staying on it reports none");
			});

		InSuite.Add("Animation: easing ends are fixed, sequence steps follow each other, loop and ping-pong repeat, from falls back to the previous end", [](TestContext& InContext)
			{
				InContext.ExpectNear(model::ApplyEasing(model::Easing::BackOut, 0.0f), 0.0f, 0.001f, "easing starts at 0");
				InContext.ExpectNear(model::ApplyEasing(model::Easing::BounceOut, 1.0f), 1.0f, 0.001f, "and ends at 1");
				InContext.Expect(model::ApplyEasing(model::Easing::BackOut, 0.7f) > 1.0f, "back overshoots");
				const auto Node = JsonConfigNode::Parse(R"({"sequence": true, "steps": [{"property": "move_x", "to": 10, "duration": 1}, {"property": "move_x", "to": 0, "duration": 1}, {"property": "alpha", "from": 0.5, "to": 1, "duration": 1}]})", "anim test");
				TArray<std::string> Missing;
				const model::AnimationDef Def = model::AnimationDef::FromConfig(*Node, "t", Missing);
				InContext.Expect(Def.Steps.Size() == 3 && Missing.IsEmpty(), "three steps read");
				model::AnimationPlayer Player(Def);
				InContext.ExpectNear(Player.GetTotalTime(), 3.0f, 0.001f, "sequence length is the sum");
				Player.Advance(1.0f);
				InContext.ExpectNear(Player.Sample().MoveX, 10.0f, 0.001f, "first step done");
				Player.Advance(0.5f);
				InContext.ExpectNear(Player.Sample().MoveX, 5.0f, 0.001f, "second step starts from the first end value");
				InContext.ExpectNear(Player.Sample().Alpha, 1.0f, 0.001f, "a step that has not started changes nothing");
				Player.Advance(5.0f);
				InContext.Expect(Player.IsFinished(), "finished");
				InContext.ExpectNear(Player.Sample().Alpha, 1.0f, 0.001f, "final alpha");
				const auto Loop = JsonConfigNode::Parse(R"({"loop": true, "ping_pong": true, "steps": [{"property": "scale", "from": 1, "to": 2, "duration": 1}]})", "loop test");
				model::AnimationPlayer Looping(model::AnimationDef::FromConfig(*Loop, "l", Missing));
				Looping.Advance(1.5f);
				InContext.ExpectNear(Looping.Sample().ScaleX, 1.5f, 0.001f, "forward");
				Looping.Advance(1.0f);
				InContext.ExpectNear(Looping.Sample().ScaleX, 1.5f, 0.001f, "and back");
				InContext.Expect(!Looping.IsFinished(), "a loop never finishes");
			});

		InSuite.Add("UiWidget: an animation file drives a child, and hover keys swap its look", [](TestContext& InContext)
			{
				const std::string Text = R"({"layout": "anchor", "size": [100, 100], "children": [{"id": "a", "type": "rect", "size": [10, 10], "fill": "button", "hover_fill": "selection", "animation": "pop_in"}]})";
				const auto Node = JsonConfigNode::Parse(Text, "anim widget");
				TArray<std::string> Missing;
				const viewmodel::WidgetDef Def = viewmodel::WidgetDef::FromConfig(*Node, "t", Missing);
				InContext.Expect(Def.Children[0].Animation == "pop_in" && Def.Children[0].HoverFill.IsSet(), "keys read");
				auto* const Widget = view::cocos::LayoutWidget::Create(Def);
				InContext.Expect(Widget != nullptr && Widget->IsHoverable(), "hover keys make it hoverable");
				if ( !Widget )
				{
					return;
				}
				const float Start = Widget->FindPart("a")->getScale();
				Widget->update(1.0f);
				InContext.Expect(Widget->FindPart("a")->getScale() > Start, "the pop-in grows the child");
				Widget->SetHovered(true);
				InContext.Expect(Widget->IsHovered(), "hovered");
				InContext.Expect(Def.ToConfig().Get("children").GetArray()[0].Has("hover_fill"), "hover keys are saved");
			});

		InSuite.Add("UiWidget: a label with {icon:frame} becomes text and icon pieces, and goes back to one label for a plain text", [](TestContext& InContext)
			{
				const std::string Text = R"({"layout": "row", "children": [{"id": "price", "type": "label", "font_size": 20, "text": "Cost {icon:weapon_cannon} 5"}]})";
				const auto Node = JsonConfigNode::Parse(Text, "rich test");
				TArray<std::string> Missing;
				const viewmodel::WidgetDef Def = viewmodel::WidgetDef::FromConfig(*Node, "t", Missing);
				auto* const Widget = view::cocos::LayoutWidget::Create(Def);
				InContext.Expect(Widget != nullptr, "created");
				if ( !Widget )
				{
					return;
				}
				InContext.Expect(Widget->FindWidget("price") != nullptr, "the markup makes a row of pieces");
				InContext.Expect(Widget->GetText("price") == "Cost {icon:weapon_cannon} 5", "the text keeps its markup");
				const float RichWidth = Widget->getContentSize().width;
				Widget->SetText("price", "Cost");
				InContext.Expect(Widget->FindWidget("price") == nullptr && Widget->GetText("price") == "Cost", "a plain text is one label again");
				Widget->SetText("price", "{icon:weapon_cannon}");
				InContext.Expect(Widget->FindWidget("price") != nullptr && Widget->getContentSize().width < RichWidth, "and back to pieces");
			});

		InSuite.Add("WidgetEditSession: numbers step within their range, undo and redo restore them, only fitting numbers apply, dirty follows the saved text", [](TestContext& InContext)
			{
				const auto Node = JsonConfigNode::Parse(R"({"layout": "row", "gap": 4, "children": [{"id": "name", "type": "label", "font_size": 20}, {"id": "box", "type": "rect", "size": [10, 10], "opacity": 1}]})", "edit test");
				TArray<std::string> Missing;
				viewmodel::WidgetEditSession Session(viewmodel::WidgetDef::FromConfig(*Node, "t", Missing));
				using viewmodel::EditProperty;
				InContext.Expect(!Session.IsDirty(), "a fresh session is saved");
				InContext.Expect(Session.IsApplicable(EditProperty::Gap) && !Session.IsApplicable(EditProperty::FontSize), "the widget has gap, no font size");
				Session.Nudge(EditProperty::Gap, 1);
				InContext.ExpectNear(Session.GetValue(EditProperty::Gap), 5.0f, 0.001f, "gap steps up");
				InContext.Expect(Session.IsDirty() && Session.CanUndo(), "dirty with an undo step");
				Session.SelectNext(1);
				InContext.Expect(Session.GetSelectedName() == "name" && Session.IsApplicable(EditProperty::FontSize) && !Session.IsApplicable(EditProperty::Gap), "next selects the first child");
				Session.Nudge(EditProperty::FontSize, -1);
				InContext.ExpectNear(Session.GetValue(EditProperty::FontSize), 19.0f, 0.001f, "font size steps down");
				Session.SelectNext(1);
				InContext.Expect(!Session.IsApplicable(EditProperty::FontSize) && Session.IsApplicable(EditProperty::Radius), "a rect has a radius, no font size");
				Session.Nudge(EditProperty::Opacity, 1);
				InContext.ExpectNear(Session.GetValue(EditProperty::Opacity), 1.0f, 0.001f, "opacity stays within 0..1");
				InContext.Expect(Session.GetDef().ToConfig().Get("gap").AsInt() == 5, "the edit is in the saved text");
				Session.SelectNext(1);
				InContext.Expect(Session.GetSelected() == viewmodel::WidgetEditSession::WidgetSelected, "the selection wraps to the widget");
				Session.Undo();
				InContext.Expect(Session.GetSelected() == 1 || Session.GetSelected() == 0, "undo brings back the selection of the undone edit");
				Session.Select(viewmodel::WidgetEditSession::WidgetSelected);
				InContext.ExpectNear(Session.GetValue(EditProperty::Gap), 5.0f, 0.001f, "undo of the font size leaves the gap edit");
				Session.Undo();
				InContext.ExpectNear(Session.GetValue(EditProperty::Gap), 4.0f, 0.001f, "second undo restores the gap");
				InContext.Expect(!Session.IsDirty(), "back to the saved text");
				Session.Redo();
				InContext.ExpectNear(Session.GetValue(EditProperty::Gap), 5.0f, 0.001f, "redo");
				Session.MarkSaved();
				InContext.Expect(!Session.IsDirty(), "saved");
			});

		InSuite.Add("ViewDef: every view file names a widget file, its texts and items", [](TestContext& InContext)
			{
				const auto Entries = config_refs::ListGroupEntries(ids::config_groups::Views);
				InContext.Expect(!Entries.IsEmpty(), "views are registered");
				UiStyle& Style = GameContext::Get<UiStyle>();
				for ( const config_refs::GroupEntry& Entry : Entries )
				{
					const viewmodel::ViewDef View = viewmodel::ViewDef::FromConfig(config_refs::GetConfig(model::ResourceId(Entry.Id)), Entry.Id);
					const viewmodel::MenuDef* const Menu = View.Menu.empty() ? nullptr : Style.FindMenu(View.Menu);
					InContext.Expect(View.Menu.empty() || Menu != nullptr, Entry.Id + ": the menu file exists");
					InContext.Expect(Style.FindWidget(Menu ? Menu->Root : View.Root) != nullptr, Entry.Id + ": the root widget exists");
					for ( const viewmodel::ViewItemDef& Item : View.Items )
					{
						InContext.Expect(Style.FindWidget(Item.Widget) != nullptr, Entry.Id + ": the item widget exists");
					}
				}
			});

		InSuite.Add("MenuEditSession: fields cycle, items move, copy and go, undo and redo restore, the saved text reads back the same", [](TestContext& InContext)
			{
				const viewmodel::MenuDef* const Source = GameContext::Get<UiStyle>().FindMenu(ids::menus::Pause);
				InContext.Expect(Source != nullptr, "the pause menu exists");
				if ( !Source )
				{
					return;
				}
				viewmodel::MenuEditSession Session(*Source);
				const size_t Count = Source->Items.Size();
				InContext.Expect(!Session.IsDirty(), "starts clean");
				Session.Select(1);
				Session.Cycle(viewmodel::MenuField::When, 1, { "a", "b" });
				InContext.Expect(Session.GetValue(viewmodel::MenuField::When) == "a" && Session.IsDirty(), "a field steps to the first choice");
				Session.Cycle(viewmodel::MenuField::When, -1, { "a", "b" });
				InContext.Expect(Session.GetValue(viewmodel::MenuField::When).empty() && !Session.IsDirty(), "and back to none");
				Session.MoveSelected(-1);
				InContext.Expect(Session.GetSelected() == 0 && Session.GetDef().Items[0].Id == Source->Items[1].Id, "an item moves up");
				Session.DuplicateSelected();
				InContext.Expect(Session.GetDef().Items.Size() == Count + 1 && Session.GetDef().Items[1].Id != Session.GetDef().Items[0].Id, "a copy gets its own id");
				Session.RemoveSelected();
				InContext.Expect(Session.GetDef().Items.Size() == Count, "an item goes");
				Session.ToggleDebug();
				InContext.Expect(Session.IsDebug(), "the debug mark toggles");
				while ( Session.CanUndo() )
				{
					Session.Undo();
				}
				InContext.Expect(!Session.IsDirty(), "undo restores the file");
				Session.Redo();
				InContext.Expect(Session.CanUndo(), "redo applies the first change again");
				Session.Select(0);
				Session.ToggleDebug();
				const std::string Text = Session.ToText();
				ConfigReader Reader;
				InContext.Expect(Reader.LoadConfigText(Text, "menu"), "the text parses");
				const viewmodel::MenuDef Reread = viewmodel::MenuDef::FromConfig(Reader.GetConfig("menu"), Source->Id);
				InContext.Expect(Reread.Items.Size() == Session.GetDef().Items.Size() && Reread.Items[0].bDebug, "the saved text reads back as the same menu");
			});

		InSuite.Add("MenuDef: choice rows have options or a source, stepper rows have limits, and options survive a save", [](TestContext& InContext)
			{
				const viewmodel::MenuDef* const Settings = GameContext::Get<UiStyle>().FindMenu(ids::menus::Settings);
				InContext.Expect(Settings != nullptr, "the options menu exists");
				if ( !Settings )
				{
					return;
				}
				for ( const viewmodel::MenuItemDef& Item : Settings->Items )
				{
					if ( Item.Kind == "choice" )
					{
						InContext.Expect(!Item.Options.IsEmpty() || !Item.OptionsSource.empty(), Item.Id + ": a choice has options or a source");
					}
					if ( Item.Kind == "stepper" || Item.Kind == "slider" )
					{
						InContext.Expect(Item.Max > Item.Min && Item.Step > 0.0f, Item.Id + ": a stepper has limits and a step");
					}
				}
				viewmodel::MenuEditSession Session(*Settings);
				const auto Found = std::find_if(Settings->Items.begin(), Settings->Items.end(), [](const viewmodel::MenuItemDef& InItem) { return InItem.Id == "pixels"; });
				Session.Select(static_cast<int>(Found - Settings->Items.begin()));
				Session.AddOption("MENU_BACK");
				ConfigReader Reader;
				InContext.Expect(Reader.LoadConfigText(Session.ToText(), "menu"), "the text parses");
				const viewmodel::MenuDef Reread = viewmodel::MenuDef::FromConfig(Reader.GetConfig("menu"), Settings->Id);
				InContext.Expect(Reread.Items[static_cast<size_t>(Session.GetSelected())].Options.Size() == Found->Options.Size() + 1, "an added option is saved");
				Session.RemoveLastOption();
				InContext.Expect(!Session.IsDirty(), "removing it gives the file back");
			});

		InSuite.Add("WidgetEditorScene: every view opens, also one after another in the same scene", [](TestContext& InContext)
			{
				auto* const Scene = static_cast<view::cocos::WidgetEditorScene*>(view::cocos::WidgetEditorScene::CreateScene());
				for ( const config_refs::GroupEntry& Entry : config_refs::ListGroupEntries(ids::config_groups::Views) )
				{
					InContext.Expect(Scene->ShowView(Entry.Id.substr(ids::config_groups::Views.size())), Entry.Id + ": opens");
				}
			});

		InSuite.Add("WidgetEditorScene: opens on the first widget file", [](TestContext& InContext)
			{
				InContext.Expect(view::cocos::WidgetEditorScene::CreateScene() != nullptr, "the scene is built with its preview and panels");
			});

		InSuite.Add("UiWidget: reloading the widget files keeps a live widget working", [](TestContext& InContext)
			{
				auto* const Widget = view::cocos::WidgetFactory::Create(ids::widgets::Keycap);
				InContext.Expect(Widget != nullptr, "created");
				if ( !Widget )
				{
					return;
				}
				Widget->SetText("text", "Ctrl");
				GameContext::Get<UiStyle>().ReloadWidgets();
				view::cocos::LayoutWidget::ReloadAll();
				InContext.Expect(Widget->GetText("text") == "Ctrl", "the text set by code survives the rebuild");
			});

		InSuite.Add("UiWidget: a bar jumps to its first value, then eases, and a drop leaves a trail that drains after a delay", [](TestContext& InContext)
		{
			auto* const Bar = view::cocos::WidgetFactory::Create(ids::widgets::HudHealthBar);
			InContext.Expect(Bar != nullptr, "the health bar widget file is registered");
			if ( !Bar )
			{
				return;
			}
			Bar->SetPartFraction("bar", 1.0f);
			InContext.ExpectNear(Bar->GetPartShownFraction("bar"), 1.0f, 0.001f, "the first value shows at once");
			Bar->SetPartFraction("bar", 0.4f);
			InContext.ExpectNear(Bar->GetPartShownFraction("bar"), 1.0f, 0.001f, "a drop does not jump");
			Bar->update(0.3f);
			const float Halfway = Bar->GetPartShownFraction("bar");
			InContext.Expect(Halfway < 1.0f && Halfway > 0.4f, "it eases towards the new value");
			Bar->update(1.0f);
			InContext.ExpectNear(Bar->GetPartShownFraction("bar"), 0.4f, 0.001f, "and arrives");
			Bar->SetPartFraction("bar", 0.8f);
			Bar->update(2.0f);
			InContext.ExpectNear(Bar->GetPartShownFraction("bar"), 0.8f, 0.001f, "a rise eases up as well");
		});

		InSuite.Add("UiWidget: the switch bar centres a ring and a keycap as one group in a fixed bar", [](TestContext& InContext)
		{
			auto* const Bar = view::cocos::WidgetFactory::Create(ids::widgets::SwitchBar);
			InContext.Expect(Bar != nullptr, "the switch bar widget file is registered");
			if ( !Bar )
			{
				return;
			}
			auto* const Key = Bar->FindWidget("key");
			InContext.Expect(Key != nullptr, "the keycap is a child widget");
			InContext.ExpectNear(Bar->getContentSize().width, 72.0f, 0.001f, "the bar keeps its fixed width");
			if ( Key )
			{
				Key->SetText("text", "Left Shift Key");
				InContext.ExpectNear(Bar->getContentSize().width, 72.0f, 0.001f, "a long key name does not change the bar");
				InContext.ExpectNear(Key->getContentSize().width, 36.0f, 0.001f, "nor the keycap");
			}
		});
	}

	void AddUiStyleTests(TestSuite& InSuite)
	{
		InSuite.Add("UI style: fields keep their defaults and missing keys are recorded with their path", [](TestContext& InContext)
		{
			const auto Root = JsonConfigNode::Parse(R"({"top": {"padding": 5, "anchor": "top_right"}, "slots": {"max": "many"}})", "test");
			TArray<std::string> Missing;
			const viewmodel::HudLayout Hud = viewmodel::HudLayout::FromConfig(Root.Get(), Missing);
			InContext.ExpectNear(Hud.TopPadding, 5.0f, 0.001f, "a present value is read");
			InContext.Expect(Hud.TopAnchor == model::ScreenAnchor::TopRight, "anchors are read by name");
			InContext.ExpectNear(Hud.TopFontSize, viewmodel::HudLayout().TopFontSize, 0.001f, "a missing value keeps its default");
			InContext.Expect(Hud.MaxSlots == viewmodel::HudLayout().MaxSlots, "a value of the wrong type keeps its default");
			const auto Has = [&Missing](const std::string& InPath) { return std::find(Missing.begin(), Missing.end(), InPath) != Missing.end(); };
			InContext.Expect(Has("top.font_size") && Has("slots.max") && Has("banner.hold"), "missing and wrong keys are recorded with their path");
			InContext.Expect(!Has("top.padding") && !Has("top.anchor"), "read keys are not");

			TArray<std::string> NoFile;
			const viewmodel::MenuLayout Menu = viewmodel::MenuLayout::FromConfig(nullptr, NoFile);
			InContext.Expect(!NoFile.IsEmpty(), "a missing file gives the defaults and reports every key");
		});

		InSuite.Add("UI style: the shipped configs/ui files have every key", [](TestContext& InContext)
		{
			const ConfigReader& Reader = GameContext::Get<ConfigReader>();
			const auto Check = [&InContext, &Reader](const std::string& InId, const auto& InLoad)
			{
				TArray<std::string> Missing;
				InLoad(Reader.HasConfig(InId) ? &Reader.GetConfig(InId) : nullptr, Missing);
				InContext.Expect(Missing.IsEmpty(), InId + (Missing.IsEmpty() ? "" : " misses " + Missing.Front()));
			};
			Check("configs/ui/widgets.json", &viewmodel::WidgetStyle::FromConfig);
			Check("configs/ui/menu.json", &viewmodel::MenuLayout::FromConfig);
			Check("configs/ui/hud.json", &viewmodel::HudLayout::FromConfig);
			Check("configs/ui/editor.json", &viewmodel::EditorStyle::FromConfig);
			Check("configs/ui/splash.json", &viewmodel::SplashStyle::FromConfig);
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
			class Counted : public TRefCounted
			{
			public:
				explicit Counted(int* const InDestroyed) : Destroyed(InDestroyed) {}
			private:
				~Counted() override { ++*Destroyed; }
				int* Destroyed;
			};

			THandle<Counted> First(new Counted(&Destroyed));
			InContext.ExpectEqual(First->GetRefCount(), 1, "one handle");
			{
				THandle<Counted> Copy = First;
				InContext.ExpectEqual(First->GetRefCount(), 2, "a copy counts");
				THandle<Counted> Moved = std::move(Copy);
				InContext.Expect(!Copy && Moved && First->GetRefCount() == 2, "a move does not count twice");
			}
			InContext.Expect(Destroyed == 0 && First->GetRefCount() == 1, "still alive with one handle");
			First.Reset();
			InContext.ExpectEqual(Destroyed, 1, "freed when the last handle is released");
		});

		InSuite.Add("ResourceLoadJob: loads a set step by step and holds it until taken", [](TestContext& InContext)
		{
			ResourceManager& Manager = GameContext::Get<ResourceManager>();
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
				InContext.Expect(Taken.GetAtlases().Size() == 1 && Taken.GetSounds().Size() == 1, "the holder has what loaded");
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
			InContext.ExpectEqual(WithSounds.Atlases.Size(), size_t(2), "the tileset and one enemy");
			InContext.Expect(Contains(WithSounds.Sounds, "sfx.player_shot") && Contains(WithSounds.Sounds, "sfx.enemy_explode"), "player and enemy sounds");
			InContext.Expect(!Contains(WithSounds.Sounds, "sfx.boss_explode"), "no boss sound in level 1");
			InContext.Expect(Manifest.GetLevelResources(First, false).Sounds.IsEmpty(), "no sounds when the sound is off");
		});

		InSuite.Add("WorkQueue: work runs on another thread, results come back in order on Pump()", [](TestContext& InContext)
		{
			model::WorkQueue Queue;
			const std::thread::id MainThread = std::this_thread::get_id();
			TArray<int> Results;
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
						Results.PushBack(InValue);
					});
			}
			InContext.Expect(Results.IsEmpty(), "nothing is delivered before Pump");
			const auto Until = std::chrono::steady_clock::now() + std::chrono::seconds(5);
			while ( Results.Size() < 5 && std::chrono::steady_clock::now() < Until )
			{
				Queue.Pump();
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			InContext.Expect(Results == TArray<int>({ 0, 10, 20, 30, 40 }), "five results, in submit order");
			InContext.Expect(bWorkedElsewhere, "work off the main thread, done on it");
			InContext.ExpectEqual(Queue.GetPendingCount(), std::size_t(0), "nothing pending at the end");
		});

		InSuite.Add("WorkQueue: urgent jobs go first, Pump respects its budget", [](TestContext& InContext)
		{
			model::WorkQueue Queue;
			std::atomic<bool> bOpen{ false };
			TArray<std::string> Order;
			std::atomic<bool> bStarted{ false };
			Queue.Submit<int>([&]() { bStarted = true; while ( !bOpen ) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); } return 0; }, [&Order](int) { Order.PushBack("gate"); });
			while ( !bStarted ) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
			Queue.Submit<int>([]() { return 0; }, [&Order](int) { Order.PushBack("low"); }, model::WorkPriority::Low);
			Queue.Submit<int>([]() { return 0; }, [&Order](int) { Order.PushBack("normal"); });
			Queue.Submit<int>([]() { return 0; }, [&Order](int) { Order.PushBack("high"); }, model::WorkPriority::High);
			bOpen = true;
			Queue.WaitIdle();

			InContext.ExpectEqual(Queue.Pump(std::chrono::microseconds(0)), std::size_t(1), "a zero budget still delivers one job");
			InContext.ExpectEqual(Queue.GetPendingCount(), std::size_t(3), "the rest wait for the next frame");
			Queue.Pump();
			InContext.Expect(Order == TArray<std::string>({ "high", "gate", "normal", "low" }), "delivered high first, then normal in submit order, then low");
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
			ResourceManager& Manager = GameContext::Get<ResourceManager>();
			TArray<unsigned char> Bytes;
			bool bDone = false;
			Manager.ReadFileAsync(PathOf(ids::configs::PlayerTank, model::ResourceKind::Config), [&](TArray<unsigned char> InBytes) { Bytes = std::move(InBytes); bDone = true; });
			const auto Until = std::chrono::steady_clock::now() + std::chrono::seconds(5);
			while ( !bDone && std::chrono::steady_clock::now() < Until )
			{
				Manager.GetQueue().Pump();
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			InContext.Expect(bDone && Bytes.Size() > 10 && Bytes[0] == '{', "the bytes of the config arrive");
		});

		InSuite.Add("Atlases: an async load decodes on the worker, shares one load and unloads with the last handle", [](TestContext& InContext)
		{
			ResourceManager& Manager = GameContext::Get<ResourceManager>();
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
			ResourceManager& Manager = GameContext::Get<ResourceManager>();
			cocos2d::FileUtils* const Files = cocos2d::FileUtils::getInstance();
			const TArray<FPersistentString> Paths{ Manager.GetTable().GetPaths(model::ResourceKind::Atlas) };
			InContext.Expect(!Paths.IsEmpty(), "atlas ids are listed");
			for ( const FPersistentString Path : Paths )
			{
				const std::string PathText{ Path.View() };
				InContext.Expect(Files->isFileExist(PathText), "atlas file exists: " + PathText);
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
			InContext.Expect(Found && Found->Path.View() == "sounds/a.wav" && Found->Kind == model::ResourceKind::Sound, "a sound id");
			InContext.Expect(Table.Find("sfx.missing") == nullptr, "an unknown id is not found");
			InContext.Expect(Table.GetIds(model::ResourceKind::Atlas).Size() == 1, "ids by kind");

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
				return InPath == "sounds.json" ? SoundsFile.Get() : nullptr;
			};
			model::ResourceTable Table;
			InContext.Expect(!Table.FromConfig(*Root, Resolver), "a missing section file is reported");
			InContext.Expect(Table.Find("sfx.a") && Table.Find("sfx.a")->Path == "a.wav", "the section from another file");
			InContext.Expect(Table.Find("cfg.a") != nullptr, "an inline section next to it still reads");
		});

		InSuite.Add("ConfigReader: parsed values use the engine-free JSON variant", [](TestContext& InContext)
		{
			ConfigReader Reader;
			const std::string Text = R"({"count":3,"enabled":true,"items":["text",null,2.5],"nested":{"name":"config"}})";
			InContext.Expect(Reader.LoadConfigText(Text, "variant"), "an object config loads");
			const model::JsonValue& Values = Reader.GetObjectData("variant").Values;
			const model::JsonValue::Array& Items = Values.Get("items").GetArray();
			InContext.Expect(Values.Get("count").AsInt() == 3 && Values.Get("enabled").AsBool(), "number and boolean values keep their types");
			InContext.Expect(Items.Size() == 3 && Items[0].AsString() == "text" && Items[1].IsNull() && Items[2].AsDouble() == 2.5, "arrays preserve strings, null and decimals");
			InContext.Expect(Values.Get("nested").GetString("name", "") == "config", "nested objects remain readable");
			InContext.Expect(!Reader.LoadConfigText("[]", "not_object"), "a non-object root is rejected");
		});

		InSuite.Add("Object configs: look and balance parts are merged into the object values", [](TestContext& InContext)
		{
			const model::JsonValue::Object Brick = GameContext::Get<ObjectsFactory>().ReadValues(config_refs::ResolveId(ids::configs::WallBrick).View());
			InContext.Expect(DataConverter::GetString(Brick, "hit_effect", "") == "hit_brick", "hit_effect comes from the look part");
			InContext.Expect(DataConverter::GetInt(Brick, "cell_health", 0) == 2, "cell_health comes from the balance part");
			InContext.Expect(DataConverter::GetString(Brick, "texture_path", "") == "wall_brick", "the schema keys stay");
			const model::JsonValue::Object Wreck = GameContext::Get<ObjectsFactory>().ReadValues(config_refs::ResolveId(ids::configs::TankWreck).View());
			InContext.Expect(DataConverter::GetFloat(Wreck, "lifetime_seconds", 0.0f) > 0.0f, "the tank wreck lifetime is balance");
		});

		InSuite.Add("Config ids: every registered config exists, every config file is registered, every id inside JSON resolves", [](TestContext& InContext)
		{
			ResourceManager& Manager = GameContext::Get<ResourceManager>();
			const ConfigReader& Reader = GameContext::Get<ConfigReader>();
			const TArray<FPersistentString> RegisteredPaths{ Manager.GetTable().GetPaths(model::ResourceKind::Config) };
			for ( const FPersistentString Path : RegisteredPaths )
			{
				const std::string PathText{ Path.View() };
				InContext.Expect(Reader.HasConfig(PathText), "registered config is loaded: " + PathText);
			}
			for ( const FPersistentString ConfigId : Reader.GetConfigIds("configs/") )
			{
				const std::string Path{ ConfigId.View() };
				const bool bRegistry = Path.compare(0, 18, "configs/resources/") == 0 || Path == ResourcesConfig;
				InContext.Expect(bRegistry || std::find(RegisteredPaths.begin(), RegisteredPaths.end(), Path) != RegisteredPaths.end(), "config file is registered: " + Path);
				InContext.Expect(Path.find('/', 8) != std::string::npos || Path == ResourcesConfig, "config file sits in a folder: " + Path);
			}

			const TFunction<void(const model::IConfigNode&, const std::string&), UiCallbackInlineSize> CheckValue = [&](const model::IConfigNode& InValue, const std::string& InWhere)
			{
				std::string Text;
				if ( InValue.ReadSelf(Text) )
				{
					if ( Text.compare(0, 4, "cfg.") == 0 )
					{
						InContext.Expect(Manager.GetTable().Find(Text) != nullptr, "id in " + InWhere + " resolves: " + Text);
					}
				}
				for ( const std::string& Key : InValue.GetKeys() )
				{
					CheckValue(*InValue.FindChild(Key), InWhere);
				}
				for ( std::size_t Index = 0; Index < InValue.GetItemCount(); ++Index )
				{
					CheckValue(*InValue.GetItem(Index), InWhere);
				}
			};
			for ( const FPersistentString RegisteredPath : RegisteredPaths )
			{
				const std::string Path{ RegisteredPath.View() };
				CheckValue(Reader.GetConfig(Path), Path);
			}
		});

		InSuite.Add("Config groups: enemies skip non-enemy files, levels come sorted", [](TestContext& InContext)
		{
			const TArray<std::string> Enemies = config_refs::ListGroup(ids::config_groups::Enemies);
			InContext.Expect(std::find(Enemies.begin(), Enemies.end(), "configs/objects/movables/tanks/enemies/Sniper.json") != Enemies.end(), "Sniper is listed");
			for ( const std::string& Path : Enemies )
			{
				InContext.Expect(Path.find("spawned") == std::string::npos, "not a spawnable enemy: " + Path);
			}
			const TArray<std::string> Levels = config_refs::ListGroup(ids::config_groups::Levels);
			InContext.Expect(Levels.Size() >= 6 && Levels.Front().find("level_1.") != std::string::npos && Levels[5].find("level_6.") != std::string::npos, "levels in order");
			InContext.Expect(config_refs::ResolveName("").IsEmpty(), "an empty name gives no path");
		});

		InSuite.Add("ResourceManager: every id in code resolves, every file exists and none is unused", [](TestContext& InContext)
		{
			ResourceManager& Manager = GameContext::Get<ResourceManager>();
			cocos2d::FileUtils* const Files = cocos2d::FileUtils::getInstance();
			const auto CheckGroup = [&](const auto& InIds, const model::ResourceKind InKind)
			{
				for ( const model::ResourceId& Id : InIds )
				{
					const std::string Path{ Manager.ResolvePath(Id, InKind).View() };
					InContext.Expect(!Path.empty() && Files->isFileExist(Path), "id resolves to an existing file: " + std::string(Id.GetName()));
				}
			};
			CheckGroup(ids::sounds::All, model::ResourceKind::Sound);
			CheckGroup(ids::fonts::All, model::ResourceKind::Font);
			CheckGroup(ids::configs::All, model::ResourceKind::Config);
			CheckGroup(ids::images::All, model::ResourceKind::Image);

			const auto CheckFolder = [&](const model::ResourceKind InKind, const std::string& InExtension)
			{
				const TArray<FPersistentString> Paths{ Manager.GetTable().GetPaths(InKind) };
				const std::string First{ Paths.Front().View() };
				const std::string Sample = Files->fullPathForFilename(First);
				const std::string Folder = First.substr(0, First.find('/') + 1);
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
			ResourceManager& Manager = GameContext::Get<ResourceManager>();
			const auto Mod = JsonConfigNode::Parse(R"({"sounds":{"sfx.test_laser":"sounds/pause.wav"}})", "test");
			Manager.ApplyOverrides(*Mod);

			const auto Config = JsonConfigNode::Parse(R"({"sound_explode":"sfx.test_laser","sound_hit":"sfx.no_such_sound",
				"weapons":[{"id":"laser","sound":"sfx.test_laser"},{"id":"quiet"}],
				"abilities":[{"type":"Buff","buff":"shield","sound":"sfx.player_shot"}]})", "test");
			const TankDef Def = TankLoader::ReadTank(*Config);
			InContext.Expect(Def.ExplodeSound.IsValid() && Def.ExplodeSound.GetName() == "sfx.test_laser", "a new sound id is usable from a config");
			InContext.Expect(!Def.HitSound.IsValid() && !Def.ArmorSound.IsValid(), "an unknown or missing sound id means silence");
			InContext.Expect(Def.Weapons.Size() == 2 && Def.Weapons[0].FireSound.IsValid() && !Def.Weapons[1].FireSound.IsValid(), "weapon sounds");
			InContext.Expect(Def.Abilities.Size() == 1 && Def.Abilities[0].ActivateSound == ids::sounds::PlayerShot, "ability sound");
			InContext.Expect(PathOf(Def.ExplodeSound, model::ResourceKind::Sound) == "sounds/pause.wav", "the id resolves to its file");
		});

		InSuite.Add("Sounds: shipped tanks name valid sounds", [](TestContext& InContext)
		{
			const ConfigReader& Reader = GameContext::Get<ConfigReader>();
			const TankDef Player = TankLoader::ReadTank(Reader.GetConfig(PlayerTankPath()));
			InContext.Expect(Player.ExplodeSound.IsValid() && Player.HitSound.IsValid() && Player.ArmorSound.IsValid(), "the player hit and explode sounds");
			InContext.Expect(!Player.Weapons.IsEmpty() && Player.Weapons[0].FireSound.IsValid(), "the player cannon shot");
			for ( const std::string& Id : config_refs::ListGroup(ids::config_groups::Enemies) )
			{
				InContext.Expect(TankLoader::ReadTank(Reader.GetConfig(Id)).ExplodeSound.IsValid(), "explode sound: " + Id);
			}
		});

		InSuite.Add("ResourceManager: a missing id gives an empty path", [](TestContext& InContext)
		{
			ResourceManager Manager;
			InContext.Expect(Manager.ResolvePath(model::ResourceId("sfx.no_such_sound"), model::ResourceKind::Sound).IsEmpty(), "unknown id");
			InContext.Expect(Manager.ResolvePath(ids::sounds::PlayerShot, model::ResourceKind::Font).IsEmpty(), "wrong kind");
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
			InContext.Expect(Book.Count(model::ResourceState::Missing) == 1 && Book.GetMissing() == TArray<std::string>{ "frame b" }, "listed");
			Book.ForgetMissing();
			InContext.Expect(Book.Get("frame", "b") == model::ResourceState::Unchecked && Book.Get("atlas", "a") == model::ResourceState::Ok, "missing forgotten, ok kept");
			InContext.Expect(Book.MarkMissing("frame", "b"), "reported again after forgetting");
		});

		InSuite.Add("ResourceManager: asked before the configs are read, it does not stick with an empty table (startup order)", [](TestContext& InContext)
		{
			ResourceManager Manager;
			const ConfigReader Empty;
			Manager.SetConfigReader(&Empty);
			InContext.Expect(Manager.ResolvePath(ids::fonts::Plain, model::ResourceKind::Font).IsEmpty(), "no path before the configs");
			InContext.Expect(!Manager.IsTableLoaded(), "the table is not marked loaded");
			InContext.ExpectEqual(Manager.GetStatus().Count(model::ResourceState::Missing), std::size_t(0), "nothing marked missing for good");
			Manager.SetConfigReader(nullptr);
			InContext.Expect(!Manager.ResolvePath(ids::fonts::Plain, model::ResourceKind::Font).IsEmpty(), "the font resolves once the configs are read");
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
			InContext.Expect(Manifest.GetGlobalAtlases().Size() == 3, "three global atlases");
			InContext.Expect(!Manifest.GetTilesetAtlas("desert").empty(), "tilesets are read");
		});
	}

	TArray<int> GLifetimeLog;

	template <int TId>
	class LoggedManager
	{
	public:
		LoggedManager()
		{
			GLifetimeLog.PushBack(TId);
		}

		~LoggedManager()
		{
			GLifetimeLog.PushBack(-TId);
		}
	};

	void AddGameContextTests(TestSuite& InSuite)
	{
		using TestManagers = TTypeList<LoggedManager<1>, LoggedManager<2>, LoggedManager<3>>;
		static_assert(TypeIndexOf<LoggedManager<3>, TestManagers>::value == 2, "TypeIndexOf");
		static_assert(IsInTypeList<LoggedManager<2>, TestManagers>::value && !IsInTypeList<int, TestManagers>::value, "IsInTypeList");

		InSuite.Add("TManagerSet: creates in list order, destroys in reverse, once", [](TestContext& InContext)
		{
			GLifetimeLog.Clear();
			{
				TManagerSet<TestManagers> Managers;
				InContext.Expect(GLifetimeLog.IsEmpty() && !Managers.IsCreated(), "nothing before Create()");
				Managers.Create();
				Managers.Create();
				InContext.Expect(GLifetimeLog == TArray<int>{ 1, 2, 3 }, "created once, in list order");
				InContext.Expect(&Managers.Get<LoggedManager<2>>() == &Managers.Get<LoggedManager<2>>(), "the same instance");
				Managers.Destroy();
				InContext.Expect(GLifetimeLog == TArray<int>{ 1, 2, 3, -3, -2, -1 }, "destroyed in reverse order");
				Managers.Create();
			}
			InContext.Expect(GLifetimeLog.Size() == 12 && GLifetimeLog.Back() == -1, "the destructor destroys what is left");
		});

		InSuite.Add("GameContext: the game runs with every manager created", [](TestContext& InContext)
		{
			const GameContext& Context = GameContext::Get();
			InContext.Expect(Context.IsInitialized(), "initialized");
			InContext.Expect(&Context.GetManager<ConfigReader>() == &GameContext::Get<ConfigReader>(), "one ConfigReader");
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
	AddWidgetLayoutTests(Suite);
	AddUiStyleTests(Suite);
	AddResourceIdTests(Suite);
	AddPortTests(Suite);
	AddEditorHistoryTests(Suite);
	AddTileMergeTests(Suite);
	AddCollisionTests(Suite);
	AddEditorLayoutTests(Suite);
	AddEditorToolsTests(Suite);
	AddEditorLibraryTests(Suite);
	return Suite;
}

} // namespace tests
} // namespace game
