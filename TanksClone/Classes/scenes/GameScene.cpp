#include "scenes/GameScene.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <optional>
#include <string_view>

#include "core/BuildInfo.h"
#include "core/ConfigReader.h"
#include "core/DataConverter.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "game/EventReport.h"
#include "game/ai/TankAi.h"
#include "model/ConfigNode.h"
#include "game/objects/Bonus.h"
#include "game/objects/Tank.h"
#include "game/objects/Wall.h"
#include "game/ObjectsFactory.h"
#include "game/ResourceManifest.h"
#include "model/Log.h"
#include "scenes/EditorScene.h"
#include "scenes/MenuScene.h"
#include "scenes/SceneUtils.h"
#include "scenes/UpgradesScene.h"
#include "model/CustomLibrary.h"
#include "model/DeathPenalty.h"
#include "model/GameEvents.h"
#include "model/MapCamera.h"
#include "model/Scheduler.h"
#include "model/SimulationClock.h"
#include "services/AchievementManager.h"
#include "services/AudioManager.h"
#include "services/ConfigRefs.h"
#include "services/GroundManager.h"
#include "services/LevelLegendManager.h"
#include "services/HoverManager.h"
#include "services/MusicManager.h"
#include "services/DifficultyManager.h"
#include "services/DisplaySettings.h"
#include "services/ProfileManager.h"
#include "services/ResourceManager.h"
#include "services/StringsManager.h"
#include "services/UpgradeManager.h"
#include "services/UiStyle.h"
#include "templates/TContainers.h"
#include "ui/ActionListener.h"
#include "ui/Background.h"
#include "ui/TileMergeView.h"
#include "ui/Hud.h"
#include "ui/LayoutWidget.h"
#include "ui/WidgetFactory.h"
#include "ui/SettingsPanel.h"
#include "ui/MenuBuilder.h"
#include "ui/Widgets.h"
#include "templates/TStoredString.h"

namespace game::view::cocos
{

namespace
{
	std::string FormatTuneValue(const float InValue)
	{
		char Buffer[16];
		std::snprintf(Buffer, sizeof(Buffer), "%.2f", InValue);
		return Buffer;
	}

	const model::DifficultyDef& CurrentDifficulty()
	{
		return GameContext::Get<DifficultyManager>().GetCurrent();
	}

	std::string_view ConfigPath(const model::ResourceId& InConfig)
	{
		return GameContext::Get<ResourceManager>().ResolvePath(InConfig, model::ResourceKind::Config).View();
	}

	bool LoadCampaignLevel(const std::string& InCampaignId, const int InPosition, LevelData& OutLevel)
	{
		const ports::IStorage& Storage = GameContext::Get().GetStorage();
		model::CustomLibrary Library;
		Library.Load(Storage);
		const model::CampaignDef* const Found = Library.FindCampaign(InCampaignId);
		if ( !Found || InPosition < 0 || InPosition >= static_cast<int>(Found->LevelIds.Size()) )
		{
			return false;
		}
		const std::string& LevelId = Found->LevelIds[InPosition];
		const std::optional<std::string> Text = Storage.ReadText(model::CustomLibrary::LevelFileName(LevelId));
		const std::string ConfigId = "custom." + LevelId;
		return Text && GameContext::Get<ConfigReader>().LoadConfigText(*Text, ConfigId) && LevelLoader::Load(ConfigId, OutLevel);
	}

	constexpr int KillScore = 100;
	constexpr int KillCoins = 10;
	constexpr int WinCoinsPerLevel = 50;
	constexpr float ResultDelayWin = 15.0f; // time left to pick up the bonuses still on the map
	constexpr int ResultActionTag = 0x7e5;
	constexpr float ResultDelayLose = 1.4f;
	constexpr int ZAiDebug = 100;
	constexpr int WallZOrder = 2;
	constexpr int ShadowZOrder = 6;
	constexpr unsigned FloorSeedShift = 101;
	constexpr float AiDebugFontSize = 11.0f;
	constexpr float AiDebugPanelX = 0.01f;
	constexpr float AiDebugPanelY = 0.8f;
	constexpr int ZHud = 50;
	constexpr int ZOverlay = 100;

	std::string Text(const std::string& InId)
	{
		return GameContext::Get<StringsManager>().GetString(InId);
	}

	model::JsonValue::Object PositionOverride(const cocos2d::Vec2& InPosition)
	{
		model::JsonValue::Object Overrides;
		Overrides.InsertOrAssign("position", model::JsonValue(model::Vec2f{ InPosition.x, InPosition.y }));
		return Overrides;
	}
}

cocos2d::Scene* GameScene::CreateScene(const int InLevelIndex)
{
	return create(InLevelIndex, Origin::Menu);
}

cocos2d::Scene* GameScene::CreateScene(const int InLevelIndex, ResourceHolder InPreloaded)
{
	return create(InLevelIndex, Origin::Menu, nullptr, true, &InPreloaded);
}

cocos2d::Scene* GameScene::CreateCustomScene(const Origin InOrigin)
{
	return create(CustomLevel, InOrigin);
}

cocos2d::Scene* GameScene::CreateCampaignScene(const std::string& InCampaignId, const int InPosition)
{
	const CampaignRef Reference{ InCampaignId, InPosition };
	return create(CustomLevel, Origin::Menu, nullptr, true, nullptr, &Reference);
}

cocos2d::Scene* GameScene::CreateForTest(const LevelData& InLevel, const bool bInWithBot)
{
	return create(CustomLevel, Origin::Menu, &InLevel, bInWithBot);
}

void GameScene::AddPauseButton(const std::string& InTitle, const TFunction<void(), UiCallbackInlineSize>& InAction)
{
	ExtraPauseButtons.EmplaceBack(InTitle, InAction);
}

bool GameScene::IsFinished() const
{
	return bFinished;
}

bool GameScene::WasWon() const
{
	return bWon;
}

float GameScene::GetElapsed() const
{
	return Elapsed;
}

const model::GameSession& GameScene::GetSession() const
{
	return Session;
}

const EnemySpawner* GameScene::GetSpawner() const
{
	return Spawner.Get();
}

const PlayerBot* GameScene::GetBot() const
{
	return Bot.Get();
}

GameScene* GameScene::create(const int InLevelIndex, const Origin InOrigin, const LevelData* const InProvided, const bool bInWithBot, ResourceHolder* const InPreloaded,
	const CampaignRef* const InCampaign)
{
	GameScene* NewGameScene = new (std::nothrow) GameScene();
	if ( NewGameScene && NewGameScene->init(InLevelIndex, InOrigin, InProvided, bInWithBot, InPreloaded, InCampaign) )
	{
		NewGameScene->autorelease();
		return NewGameScene;
	}
	CC_SAFE_DELETE(NewGameScene);
	return nullptr;
}

bool GameScene::IsCustom() const
{
	return LevelIndex == CustomLevel;
}

bool GameScene::init(const int InLevelIndex, const Origin InOrigin, const LevelData* const InProvided, const bool bInWithBot, ResourceHolder* const InPreloaded,
	const CampaignRef* const InCampaign)
{
	if ( !Scene::init() )
	{
		return false;
	}

	LevelIndex = InLevelIndex;
	OpenedFrom = InOrigin;
	if ( InCampaign )
	{
		Campaign = *InCampaign;
	}

	bool CurrentbLoaded = false;
	if ( InProvided )
	{
		Level = *InProvided;
		CurrentbLoaded = LevelLoader::Validate(Level).empty();
		if ( Level.Waves.IsEmpty() )
		{
			// Same as the loader does for the simple form of a level.
			Wave CurrentWave;
			CurrentWave.MaxAlive = Level.MaxAlive;
			CurrentWave.Interval = Level.SpawnInterval;
			CurrentWave.Entries.PushBack({ tiles::EnemySpawn, Level.TotalEnemies });
			Level.Waves.PushBack(CurrentWave);
		}
		bTestRun = true;
		if ( bInWithBot )
		{
			Bot.Reset(new PlayerBot());
		}
	}
	else if ( IsCampaignLevel() )
	{
		CurrentbLoaded = LoadCampaignLevel(Campaign.Id, Campaign.Position, Level);
	}
	else
	{
		CurrentbLoaded = IsCustom() ? LevelLoader::LoadCustom(Level) : LevelLoader::Load(LevelLoader::BuiltinId(InLevelIndex), Level);
	}
	if ( !CurrentbLoaded )
	{
		return false;
	}
	const ResourceManifest Manifest;
	if ( InPreloaded )
	{
		Resources.Reset(new ResourceHolder(std::move(*InPreloaded)));
	}
	else
	{
		Resources.Reset(new ResourceHolder());
		const ResourceSet Needed = Manifest.GetLevelResources(Level, GameContext::Get<AudioManager>().IsEnabled());
		for ( const std::string& CurrentAtlas : Needed.Atlases )
		{
			Resources->AddAtlas(CurrentAtlas);
		}
		for ( const std::string& CurrentSound : Needed.Sounds )
		{
			Resources->AddSound(CurrentSound);
		}
	}
	Tileset = Manifest.GetTilesetName(Level.Tileset);

	Playfield = cocos2d::Rect(0, 0, Level.GetColumnCount() * TileSize, Level.GetRowCount() * TileSize);
	if ( !bTestRun )
	{
		for ( Wave& CurrentWave : Level.Waves )
		{
			for ( WaveEntry& Entry : CurrentWave.Entries )
			{
				Entry.Count = model::DifficultyBook::ScaleCount(Entry.Count, CurrentDifficulty().SpawnCount);
			}
		}
	}
	Session.TotalEnemies = LevelLoader::GetTotalEnemies(Level);

	Session.LevelName = Text(Level.Name);

	addChild(widgets::CreateScreenBackground(), -1);

	// The playfield sits above the player's panel; everything in it uses its own coordinates.
	PlayClip = cocos2d::ClippingRectangleNode::create(Playfield);
	PlayClip->setPosition(0, BottomHudHeight);
	addChild(PlayClip, 1);
	PlayArea = Node::create();
	PlayClip->addChild(PlayArea);
	const model::TileMergeSettings MergeSettings = LoadTileMergeSettings();
	const unsigned MergeSeed = IsCustom() ? 7u : static_cast<unsigned>(InLevelIndex + 1);
	PlayArea->addChild(CreateBackground(MergeSeed, Tileset, Level.GetColumnCount(), Level.GetRowCount(), Level.Ground, GameContext::Get<LevelLegendManager>().Get(), MergeSettings.IsEnabled()), ZBackground);

	if ( MergeSettings.IsEnabled() )
	{
		FloorMerge = CreateFloorMerge(MergeSettings, MergeSeed);
		PlayArea->addChild(FloorMerge, ZBackground);
	}

	World = Node::create();
	PlayArea->addChild(World, 1);
	if ( MergeSettings.IsEnabled() )
	{
		WallMerge = TileMergeView::Create(MergeSettings, Level.GetColumnCount(), Level.GetRowCount(), MergeSeed, MergeSettings.GetSubdivision());
		cocos2d::Node* const ShadowLayer = Node::create();
		World->addChild(ShadowLayer, ShadowZOrder);
		WallMerge->SetShadowParent(ShadowLayer);
		World->addChild(WallMerge, WallZOrder);
	}
	GameContext::Get<model::GameEvents>().Clear();
	GameContext::Get<model::Scheduler>().Clear();
	FTransientString::ClearTransientStrings();

	addChild(Hud::create(&Session), ZHud);

	model::GroundMap Floor;
	Floor.Build(Level.Ground, GameContext::Get<LevelLegendManager>().Get(), Level.GetColumnCount(), Level.GetRowCount());
	GameContext::Get<GroundManager>().Set(std::move(Floor), TileSize);
	BuildLevel();
	SpawnPlayer(PlayerSpawn);
	CameraFocus = { PlayerSpawn.x, PlayerSpawn.y };
	UpdateCamera();
	if ( !bTestRun )
	{
		for ( Wave& CurrentWave : Level.Waves )
		{
			CurrentWave.Interval *= CurrentDifficulty().SpawnInterval;
		}
	}
	if ( const model::IConfigNode* const DropsNode = GameContext::Get<ConfigReader>().GetConfig(ConfigPath(ids::configs::Drops)).FindChild("drops") )
	{
		Drops.Load(*DropsNode);
	}
	Spawner.Reset(new EnemySpawner(World, Playfield, Level, EnemySpawns));
	Spawner->OnSpawned = [this](Tank& InEnemy) { SetUpEnemy(InEnemy, true); };
	if ( !Level.Script.IsEmpty() )
	{
		Script = MakeUnique<SceneScript>(Level.Script, Level.GetRowCount(), [this](const std::string& InText) { ShowBanner(Text(InText)); });
		Script->SetCounterSource([this](const std::string& InName)
		{
			const TMap<FPersistentString, int> Counters = { { "kills", Session.Kills }, { "defeated", Session.Defeated }, { "pickups", Session.DropsPicked }, { "wave", Session.WaveNumber } };
			auto* const Found = Counters.Find(InName);
			return Found ? *Found : 0;
		});
		Script->SetPlayerSource([this]() -> const Tank* { return Player; });
		Script->SetEndLevelHandler([this](const bool bInWon) { Finish(bInWon, bInWon ? ResultDelayWin : ResultDelayLose); });
	}
	Spawner->OnWaveStarted = [this](const int InWave, const int InWaveCount)
	{
		Session.WaveNumber = InWave;
		Session.WaveCount = InWaveCount;
		if ( InWaveCount > 1 && InWave == InWaveCount )
		{
			GameContext::Get<MusicManager>().Request(model::MusicMood::Intense);
		}
		if ( InWaveCount > 1 )
		{
			ShowBanner(Text("WAVE_WORD") + " " + std::to_string(InWave) + "/" + std::to_string(InWaveCount));
		}
	};

	AddActionHandler(this, [this](const model::InputAction InAction)
	{
		if ( bFinished )
		{
			if ( InAction == model::InputAction::Pause || InAction == model::InputAction::UiBack )
			{
				SkipResultDelay();
			}
			return;
		}
		if ( InAction == model::InputAction::Pause )
		{
			SetPaused(true);
		}
		else if ( InAction == model::InputAction::UiBack && bPaused )
		{
			OnBackKey();
		}
	}, model::InputContext::Gameplay);
	if constexpr ( bHasDebugTools )
	{
		AiDebugLayer = Node::create();
		World->addChild(AiDebugLayer, ZAiDebug);
		AiDebugPanel = widgets::CreateLabel("", AiDebugFontSize, widgets::colors::Accent(), false);
		AiDebugPanel->setAnchorPoint(cocos2d::Vec2(0.0f, 1.0f));
		const cocos2d::Vec2 Origin = cocos2d::Director::getInstance()->getVisibleOrigin();
		const cocos2d::Size Visible = cocos2d::Director::getInstance()->getVisibleSize();
		AiDebugPanel->setPosition(Origin + cocos2d::Vec2(Visible.width * AiDebugPanelX, Visible.height * AiDebugPanelY));
		AiDebugPanel->setVisible(false);
		addChild(AiDebugPanel, ZHud + 1);
		AddActionHandler(this, [this](const model::InputAction InAction)
		{
			if ( InAction == model::InputAction::ToggleAiDebug )
			{
				bShowAiDebug = !bShowAiDebug;
			}
			if ( InAction == model::InputAction::ToggleTileDebug )
			{
				BuildTileDebugPanel();
			}
		});
	}

	ShowBanner(Text(Level.Name));
	GameContext::Get<AudioManager>().Play(ids::sounds::LevelStart);
	if ( Level.Music.empty() )
	{
		GameContext::Get<MusicManager>().Request(model::MusicMood::Calm, true);
	}
	else
	{
		GameContext::Get<MusicManager>().PlayTrack(Level.Music);
	}
	scheduleUpdate();
	return true;
}

void GameScene::onEnterTransitionDidFinish()
{
	TypedScene::onEnterTransitionDidFinish();
	// After the transition: the previous level's onExit (which restores the menu rate) has already run.
	ApplyFpsLimit(GameContext::Get<ProfileManager>().Get().FpsLimit);
}

void GameScene::onExit()
{
	if ( bPaused )
	{
		cocos2d::Director::getInstance()->resume();
		bPaused = false;
		GameContext::Get<MusicManager>().SetPaused(false);
	}
	GameContext::Get<GroundManager>().Clear();
	ApplyMenuFrameRate();
	Scene::onExit();
}

void GameScene::BuildLevel()
{
	const model::LevelLegend& Legend = GameContext::Get<LevelLegendManager>().Get();
	const int Rows = Level.GetRowCount();
	const int Columns = Level.GetColumnCount();
	for ( int Row = 0; Row < Rows; ++Row )
	{
		const float Y = (Rows - Row - 0.5f) * TileSize;
		for ( int Column = 0; Column < Columns; ++Column )
		{
			const cocos2d::Vec2 Position((Column + 0.5f) * TileSize, Y);
			const char Tile = Level.Rows[Row][Column];
			const model::LegendEntry* const Entry = Legend.Find(Tile);
			if ( !Entry )
			{
				continue;
			}
			const model::ResourceId Config(Entry->Config);
			switch ( Entry->Role )
			{
				case model::TileRole::Wall:
				{
					GameObject* const Wall = SpawnWall(Config, Position);
					if ( Wall && Entry->bIsBase )
					{
						Wall->SetOnKilledCallback([this](GameObject&)
						{
							LoseReasonKey = "RESULT_BASE_LOST";
							Finish(false, ResultDelayLose);
						});
					}
					break;
				}
				case model::TileRole::Pickup:
					SpawnCrate(Position, ConfigPath(Config));
					break;
				case model::TileRole::Crate:
					SpawnCrate(Position, "");
					break;
				case model::TileRole::Prop:
					GameContext::Get<ObjectsFactory>().CreateFromConfig(ConfigPath(Config), World, PositionOverride(Position));
					break;
				case model::TileRole::Player:
					PlayerSpawn = Position;
					break;
				case model::TileRole::Spawner:
					EnemySpawns[Tile].PushBack(Position);
					break;
				case model::TileRole::Empty:
					break;
			}
		}
	}
	for ( const model::LevelObject& Object : Level.Layers.GetObjects() )
	{
		const cocos2d::Vec2 Position((Object.Column + 0.5f) * TileSize, (Rows - Object.Row - 0.5f) * TileSize);
		SpawnObject(model::ResourceId(Object.Config), Position, 0);
	}
}

void GameScene::SpawnObject(const model::ResourceId& InConfig, const cocos2d::Vec2& InPosition, int /*InZOrder*/)
{
	GameContext::Get<ObjectsFactory>().CreateFromConfig(ConfigPath(InConfig), World, PositionOverride(InPosition));
}

GameObject* GameScene::SpawnWall(const model::ResourceId& InConfig, const cocos2d::Vec2& InPosition)
{
	const std::string_view Path = ConfigPath(InConfig);
	const model::IConfigNode& ConfigData = GameContext::Get<ConfigReader>().GetConfig(Path);
	model::JsonValue::Object Overrides = PositionOverride(InPosition);
	if ( ConfigData.GetValueOr("tileset_frame", true) )
	{
		Overrides.InsertOrAssign("texture_path", model::JsonValue(ResourceManifest::GetTilesetFrame(Tileset, ConfigData.GetValueOr("texture_path", ""))));
	}
	GameObject* const Created = GameContext::Get<ObjectsFactory>().CreateFromConfig(Path, World, Overrides).Get();
	Wall* const CreatedWall = ObjectCast<Wall>(Created);
	if ( CreatedWall && CreatedWall->HasMergeMaterial() && WallMerge )
	{
		CreatedWall->SetMergeView(WallMerge);
	}
	return Created;
}

void GameScene::ApplyTuning()
{
	if ( FloorMerge )
	{
		FloorMerge->ApplySettings(TuneSettings);
	}
	if ( WallMerge )
	{
		WallMerge->ApplySettings(TuneSettings);
	}
}

void GameScene::BuildTileDebugPanel()
{
	if ( TileDebugPanel )
	{
		TileDebugPanel->removeFromParent();
		TileDebugPanel = nullptr;
		return;
	}
	if ( !WallMerge )
	{
		return;
	}
	struct Row
	{
		const char* Caption;
		float Min;
		float Max;
		float (*Get)(const model::TileMergeSettings&);
		void (*Set)(model::TileMergeSettings&, float);
	};
	static const Row Rows[]{
		{ "size", 0.6f, 1.0f, [](const model::TileMergeSettings& S) { return S.GetSize(); }, [](model::TileMergeSettings& S, const float V) { S.SetSize(V); } },
		{ "hard border", 0.0f, 1.0f, [](const model::TileMergeSettings& S) { return S.GetHardBorder(); }, [](model::TileMergeSettings& S, const float V) { S.SetHardBorder(V); } },
		{ "plaster", 0.0f, 1.0f, [](const model::TileMergeSettings& S) { return S.GetPlaster(); }, [](model::TileMergeSettings& S, const float V) { S.SetPlaster(V); } },
		{ "cracks", 0.0f, 1.0f, [](const model::TileMergeSettings& S) { return S.GetCracks(); }, [](model::TileMergeSettings& S, const float V) { S.SetCracks(V); } },
		{ "wall corners", 0.0f, 1.0f, [](const model::TileMergeSettings& S) { return S.GetMaterial(model::MergeMaterial::Brick).CornerSharpness; }, [](model::TileMergeSettings& S, const float V) { S.SetCornerSharpness(model::MergeMaterial::Brick, V); S.SetCornerSharpness(model::MergeMaterial::Stone, V); S.SetCornerSharpness(model::MergeMaterial::Wood, V); S.SetCornerSharpness(model::MergeMaterial::Road, V); } },
		{ "roundness", 0.0f, 1.0f, [](const model::TileMergeSettings& S) { return S.GetRoundness(); }, [](model::TileMergeSettings& S, const float V) { S.SetRoundness(V); } },
		{ "floor hard border", 0.0f, 1.0f, [](const model::TileMergeSettings& S) { return S.GetFloorHardBorder(); }, [](model::TileMergeSettings& S, const float V) { S.SetFloorHardBorder(V); } },
		{ "side face px", 0.0f, 64.0f, [](const model::TileMergeSettings& S) { return S.GetSideFacePx(); }, [](model::TileMergeSettings& S, const float V) { S.SetSideFacePx(V); } },
		{ "shadow opacity", 0.0f, 1.0f, [](const model::TileMergeSettings& S) { return S.GetShadowOpacity(); }, [](model::TileMergeSettings& S, const float V) { S.SetShadowOpacity(V); } },
		{ "shadow right px", 0.0f, 40.0f, [](const model::TileMergeSettings& S) { return S.GetShadowOffsetPx(); }, [](model::TileMergeSettings& S, const float V) { S.SetShadowOffsetPx(V); } },
		{ "shadow down px", 0.0f, 64.0f, [](const model::TileMergeSettings& S) { return S.GetShadowLengthPx(); }, [](model::TileMergeSettings& S, const float V) { S.SetShadowLengthPx(V); } },
		{ "overlay", 0.0f, 0.6f, [](const model::TileMergeSettings& S) { return S.GetOverlayAmount(); }, [](model::TileMergeSettings& S, const float V) { S.SetOverlayAmount(V); } },
	};
	if ( !bTuneReady )
	{
		TuneSettings = LoadTileMergeSettings();
		bTuneReady = true;
	}
	TileDebugPanel = WidgetFactory::Create(ids::widgets::TileMergeDebug);
	TileDebugPanel->SetText("title", "Tile merge (F5 closes)");
	const TArray<LayoutWidget*> Items = TileDebugPanel->SetListCount("rows", std::size(Rows));
	for ( std::size_t Index = 0; Index < Items.Size(); ++Index )
	{
		const Row& Current = Rows[Index];
		LayoutWidget* const Item = Items[Index];
		Item->SetText("caption", Current.Caption);
		LayoutWidget* const Slider = Item->FindWidget("slider");
		const auto ShowValue = [Item, &Current](const float InValue)
		{
			Item->SetText("value", FormatTuneValue(InValue));
		};
		const float Start = Current.Get(TuneSettings);
		ShowValue(Start);
		Slider->SetSliderValue((Start - Current.Min) / (Current.Max - Current.Min));
		Slider->SetSliderHandler([this, Item, &Current](const float InFraction)
		{
			const float Value = Current.Min + InFraction * (Current.Max - Current.Min);
			Current.Set(TuneSettings, Value);
			Item->SetText("value", FormatTuneValue(Current.Get(TuneSettings)));
			ApplyTuning();
		});
	}
	const cocos2d::Vec2 Origin = cocos2d::Director::getInstance()->getVisibleOrigin();
	const cocos2d::Size Visible = cocos2d::Director::getInstance()->getVisibleSize();
	TileDebugPanel->setPosition(Origin + cocos2d::Vec2(Visible.width * AiDebugPanelX + TileDebugPanel->getContentSize().width / 2, Visible.height * AiDebugPanelY - TileDebugPanel->getContentSize().height / 2));
	addChild(TileDebugPanel, ZHud + 2);
}

model::TileMergeSettings GameScene::LoadTileMergeSettings() const
{
	model::TileMergeSettings Settings;
	if ( const model::IConfigNode* const Node = config_refs::FindChild(ids::configs::TileMerge, "tile_merge") )
	{
		Settings.Load(*Node, Tileset);
	}
	return Settings;
}

TileMergeView* GameScene::CreateFloorMerge(const model::TileMergeSettings& InSettings, const unsigned InSeed) const
{
	TileMergeView* const View = TileMergeView::Create(InSettings, Level.GetColumnCount(), Level.GetRowCount(), InSeed + FloorSeedShift, 1);
	const model::LevelLegend& Legend = GameContext::Get<LevelLegendManager>().Get();
	for ( int Row = 0; Row < Level.GetRowCount() && Row < static_cast<int>(Level.Ground.Size()); ++Row )
	{
		for ( int Column = 0; Column < Level.GetColumnCount() && Column < static_cast<int>(Level.Ground[Row].size()); ++Column )
		{
			const model::GroundEntry* const Entry = Legend.FindGround(Level.Ground[Row][Column]);
			model::MergeMaterial Material = model::MergeMaterial::Water;
			if ( Entry && model::TileMergeSettings::FindMaterial(Entry->Merge, Material) )
			{
				View->SetCell(Column, Row, Material, 1.0f);
			}
		}
	}
	return View;
}

namespace
{
	model::DeathPenalty LoadDeathPenalty()
	{
		model::DeathPenalty Penalty;
		if ( const model::IConfigNode* const Node = config_refs::FindChild(ids::configs::DeathPenalty, "death_penalty") )
		{
			Penalty.Load(*Node);
		}
		return Penalty;
	}

	int ScaleCoins(const int InCoins)
	{
		return static_cast<int>(std::lround(InCoins * CurrentDifficulty().Coins));
	}

	int GetLevelDeaths(const int InLevel)
	{
		if ( !CurrentDifficulty().bDeathPenalty )
		{
			return 0;
		}
		const auto& Deaths = GameContext::Get<ProfileManager>().Get().LevelDeaths;
		auto* const Found = Deaths.Find(InLevel);
		return Found ? *Found : 0;
	}
}

void GameScene::SpawnPlayer(const cocos2d::Vec2& InPosition)
{
	auto NewPlayer = GameContext::Get<ObjectsFactory>().CreateFromConfig<Tank>(ConfigPath(ids::configs::PlayerTank), World, PositionOverride(InPosition));
	if ( !NewPlayer )
	{
		model::Log::Error("GameScene: could not create the player tank");
		return;
	}

	NewPlayer->SetBounds(Playfield);
	GameContext::Get<UpgradeManager>().ApplyTo(*NewPlayer);
	if ( !IsCustom() )
	{
		const int Deaths = GetLevelDeaths(LevelIndex);
		const model::DeathPenalty Penalty = LoadDeathPenalty();
		NewPlayer->SetHealth(static_cast<int>(std::ceil(NewPlayer->GetMaxHealth() * Penalty.GetHealthShare(Deaths))));
		if ( !NewPlayer->HasUnlimitedAmmo() )
		{
			NewPlayer->SetAmmo(static_cast<int>(std::ceil(NewPlayer->GetMaxAmmo() * Penalty.GetAmmoShare(Deaths))));
		}
	}
	PlayerHealth = NewPlayer->GetHealth();
	NewPlayer->SetOnKilledCallback([this](GameObject&)
	{
		Player = nullptr;
		OnPlayerKilled();
	});
	Player = NewPlayer.Get();
}

void GameScene::ShowBanner(const std::string& InText)
{
	const viewmodel::HudLayout& HudStyle = GameContext::Get<UiStyle>().GetHud();
	auto BannerLabel = widgets::CreateLabel(InText, HudStyle.BannerFontSize, widgets::colors::Accent(), true);
	BannerLabel->setPosition(Playfield.getMidX(), BottomHudHeight + Playfield.getMidY() + TileSize * 2);
	BannerLabel->setOpacity(0);
	addChild(BannerLabel, ZOverlay - 1);
	BannerLabel->runAction(cocos2d::Sequence::create(
		cocos2d::FadeIn::create(HudStyle.BannerFadeIn), cocos2d::DelayTime::create(HudStyle.BannerHold), cocos2d::FadeOut::create(HudStyle.BannerFadeOut), cocos2d::RemoveSelf::create(), nullptr));
}

void GameScene::update(const float InDelta)
{
	if ( bFinished )
	{
		return;
	}

	Elapsed += InDelta;
	UpdateCamera();
	if ( Player )
	{
		bPlayerHurt = bPlayerHurt || Player->GetHealth() < PlayerHealth;
		PlayerHealth = Player->GetHealth();
	}
	if ( Bot )
	{
		Bot->Update(InDelta);
	}
	if ( Spawner )
	{
		Spawner->Update(InDelta);
		if ( Spawner->IsDone() )
		{
			Finish(true, ResultDelayWin);
		}
	}
	if ( Script )
	{
		Script->Update(InDelta);
	}
	UpdateAiDebug();
}

void GameScene::UpdateCamera()
{
	if ( Player && Player->GetVisualNode() )
	{
		const cocos2d::Vec2 Position = Player->GetVisualNode()->getPosition();
		CameraFocus = { Position.x, Position.y };
	}
	const model::Vec2f MapSize = { Playfield.size.width, Playfield.size.height };
	const float Scale = model::MapCamera::ComputeScale(GameContext::Get<ProfileManager>().Get().MapScale, MapSize, MapSize);
	const model::Vec2f Offset = model::MapCamera::ComputeOffset(CameraFocus, Scale, MapSize, MapSize);
	PlayArea->setScale(Scale);
	PlayArea->setPosition(Offset.X, Offset.Y);
	PlayClip->setClippingEnabled(Scale != 1.0f);
}

void GameScene::UpdateAiDebug()
{
	if ( !AiDebugLayer )
	{
		return;
	}
	size_t Used = 0;
	std::string Panel;
	if ( bShowAiDebug )
	{
		GameContext::Get<ObjectsFactory>().ForEachObject<Tank>([this, &Used, &Panel](Tank& InTank)
		{
			if ( InTank.IsDestroying() || !InTank.GetAi() || !InTank.GetVisualNode() )
			{
				return;
			}
			if ( Used == AiDebugLabels.Size() )
			{
				cocos2d::Label* const NewLabel = widgets::CreateLabel("", AiDebugFontSize, widgets::colors::Accent(), false);
				AiDebugLayer->addChild(NewLabel);
				AiDebugLabels.PushBack(NewLabel);
			}
			cocos2d::Label* const CurrentLabel = AiDebugLabels[Used++];
			const std::string Tag = "#" + std::to_string(Used) + " " + std::string(model::GetTacticName(InTank.GetAi()->GetTactic()));
			CurrentLabel->setString(Tag);
			Panel += Tag + "  " + std::string(InTank.GetName().View()) + "  hp " + std::to_string(InTank.GetHealth()) + "/" + std::to_string(InTank.GetMaxHealth())
				+ "  arm " + std::to_string(InTank.GetArmor()) + "  dmg " + std::to_string(InTank.GetDamage())
				+ "  spd " + std::to_string(static_cast<int>(InTank.GetSpeed())) + "  phase " + std::to_string(InTank.GetPhase())
				+ (InTank.GetReloadRemaining() > 0.0f ? "  reload" : "");
			for ( const BuffSet::Active& CurrentBuff : InTank.GetBuffs().GetActive() )
			{
				Panel += "  +" + std::string(CurrentBuff.Def.Id.View());
			}
			for ( size_t Index = 0; Index < InTank.GetAbilityCount(); ++Index )
			{
				const Ability* const CurrentAbility = InTank.GetAbility(Index);
				if ( CurrentAbility && CurrentAbility->GetCooldownRemaining() > 0.0f )
				{
					Panel += "  cd" + std::to_string(Index) + " " + std::to_string(static_cast<int>(std::ceil(CurrentAbility->GetCooldownRemaining()))) + "s";
				}
			}
			Panel += "\n";
			CurrentLabel->setPosition(InTank.GetVisualNode()->getPosition() + cocos2d::Vec2(0.0f, TileSize * 0.75f));
			CurrentLabel->setVisible(true);
		});
	}
	for ( size_t Index = Used; Index < AiDebugLabels.Size(); ++Index )
	{
		AiDebugLabels[Index]->setVisible(false);
	}
	AiDebugPanel->setString(Panel);
	AiDebugPanel->setVisible(bShowAiDebug);
}

void GameScene::OnEnemyKilled(GameObject& InEnemy, const bool bInCountsForWin)
{
	if ( bFinished )
	{
		return;
	}

	const bool bPlayerKill = InEnemy.GetLastDamageSource() != ETeam::Enemy;
	if ( bInCountsForWin && bPlayerKill )
	{
		++Session.Defeated;
		if ( Session.TotalEnemies > 0 && Session.Defeated >= GameContext::Get<MusicManager>().GetIntenseProgress() * Session.TotalEnemies )
		{
			GameContext::Get<MusicManager>().Request(model::MusicMood::Intense);
		}
	}
	else if ( bInCountsForWin && Spawner )
	{
		Spawner->SendReplacement();
	}
	auto CurrentTank = ObjectCast<Tank>(&InEnemy);
	events::Report("enemy.destroyed");
	if ( bPlayerKill )
	{
		events::Report("enemy.killed");
		++Session.Kills;
		Session.Score += CurrentTank ? CurrentTank->GetScoreReward() : KillScore;
		if ( !IsCustom() )
		{
			Session.Coins += ScaleCoins(CurrentTank ? CurrentTank->GetCoinReward() : KillCoins);
		}
	}

	if ( CurrentTank )
	{
		DropPickup(*CurrentTank);
		SpawnSplitParts(*CurrentTank);
	}

}

void GameScene::SetUpEnemy(Tank& InEnemy, const bool bInCountsForWin)
{
	if ( !bTestRun )
	{
		InEnemy.ApplyDifficulty(CurrentDifficulty());
	}
	if ( InEnemy.GetAi() )
	{
		InEnemy.GetAi()->ApplyFeatures(Level.AiFeatures);
	}
	InEnemy.SetOnKilledCallback([this, bInCountsForWin](GameObject& InObject) { OnEnemyKilled(InObject, bInCountsForWin); });
	InEnemy.SetOnPhaseCallback([this](Tank& InBoss, const TankPhaseDef& InPhase)
	{
		if ( !bFinished )
		{
			SpawnBeside(InBoss, InPhase.SummonConfig.View(), InPhase.SummonCount, InPhase.SummonSpacingTiles);
		}
	});
	if ( InEnemy.HasPhases() )
	{
		GameContext::Get<MusicManager>().Request(model::MusicMood::Intense);
	}
	events::Report("enemy.spawned");
	if ( Script )
	{
		Script->OnEnemySpawned(InEnemy);
	}
}

void GameScene::SpawnSplitParts(const Tank& InEnemy)
{
	SpawnBeside(InEnemy, InEnemy.GetSplitInto().View(), InEnemy.GetSplitCount(), InEnemy.GetSplitSpacingTiles());
}

void GameScene::SpawnBeside(const Tank& InEnemy, const std::string_view InConfig, const int InCount, const float InSpacingTiles)
{
	if ( InCount <= 0 || InConfig.empty() || !InEnemy.GetVisualNode() )
	{
		return;
	}
	const cocos2d::Vec2 Center = InEnemy.GetVisualNode()->getPosition();
	const cocos2d::Vec2 Side(InEnemy.GetDirection().y, -InEnemy.GetDirection().x);
	const float Spacing = TileSize * InSpacingTiles;
	for ( int Index = 0; Index < InCount; ++Index )
	{
		const float Offset = (Index - (InCount - 1) * 0.5f) * Spacing;
		auto Part = GameContext::Get<ObjectsFactory>().CreateFromConfig<Tank>(InConfig, World, PositionOverride(Center + Side * Offset));
		if ( !Part )
		{
			continue;
		}
		Part->SetBounds(Playfield);
		Part->TurnTo(model::TurnState::Normalize(InEnemy.GetVisualNode()->getRotation()));
		SetUpEnemy(*Part, false);
	}
}

void GameScene::DropPickup(const Tank& InEnemy)
{
	const float ChanceScale = bTestRun ? 1.0f : CurrentDifficulty().Drops;
	const std::optional<std::string> Pickup = Drops.Roll(std::string(InEnemy.GetDropTable().View()), model::GameRandom(), ChanceScale);
	if ( !Pickup || !InEnemy.GetVisualNode() )
	{
		return;
	}
	++Session.Drops;
	SpawnPickup(config_refs::ResolveName(*Pickup).View(), InEnemy.GetVisualNode()->getPosition());
}

void GameScene::SpawnCrate(const cocos2d::Vec2& InPosition, const std::string_view InContents)
{
	model::JsonValue::Object Overrides = PositionOverride(InPosition);
	if ( !InContents.empty() )
	{
		Overrides.InsertOrAssign("contents", model::JsonValue(std::string(InContents)));
	}
	auto Crate = GameContext::Get<ObjectsFactory>().CreateFromConfig<GameObject>(ConfigPath(ids::configs::Crate), World, Overrides);
	if ( !Crate )
	{
		return;
	}
	Crate->SetOnKilledCallback([this, InPosition](GameObject& InCrate)
	{
		const Wall* const Box = ObjectCast<Wall>(&InCrate);
		if ( !Box )
		{
			return;
		}
		std::optional<std::string> Pickup;
		if ( !Box->GetContents().empty() )
		{
			Pickup = Box->GetContents();
		}
		else
		{
			Pickup = Drops.Roll(Box->GetDropTable(), model::GameRandom());
			if ( Pickup )
			{
				Pickup = std::string(config_refs::ResolveName(*Pickup).View());
			}
		}
		if ( Pickup )
		{
			SpawnPickup(*Pickup, InPosition);
		}
	});
}

void GameScene::SpawnPickup(const std::string_view InConfig, const cocos2d::Vec2& InPosition)
{
	model::JsonValue::Object Overrides = PositionOverride(InPosition);
	Overrides.InsertOrAssign("zorder", model::JsonValue(ZBonus));
	auto Dropped = GameContext::Get<ObjectsFactory>().CreateFromConfig<Bonus>(InConfig, World, Overrides);
	if ( Dropped )
	{
		Dropped->SetOnPicked([this]()
		{
			++Session.DropsPicked;
			events::Report("pickup.taken");
		});
		Dropped->SetOnCoins([this](const int InAmount)
		{
			if ( !IsCustom() )
			{
				Session.Coins += ScaleCoins(InAmount);
			}
		});
	}
}

void GameScene::OnPlayerKilled()
{
	Finish(false, ResultDelayLose);
}

void GameScene::Finish(const bool bInWon, const float InDelay)
{
	if ( bFinished )
	{
		return;
	}
	bFinished = true;
	bWon = bInWon;

	if ( bTestRun )
	{
		// A test run: report at once, no result screen and nothing saved.
		if ( OnFinished )
		{
			OnFinished(bInWon);
		}
		return;
	}

	cocos2d::Action* const Delayed = cocos2d::Sequence::create(
		cocos2d::DelayTime::create(InDelay),
		cocos2d::CallFunc::create([this, bInWon]() { ShowResult(bInWon); }),
		nullptr);
	Delayed->setTag(ResultActionTag);
	runAction(Delayed);
}

void GameScene::SkipResultDelay()
{
	if ( getActionByTag(ResultActionTag) )
	{
		stopActionByTag(ResultActionTag);
		ShowResult(bWon);
	}
}

void GameScene::ShowResult(const bool bInWon)
{
	TArray<std::string> NewAchievements;
	bool bCampaignHasNext = false;
	if ( IsCampaignLevel() )
	{
		ports::IStorage& Storage = GameContext::Get().GetStorage();
		model::CustomLibrary Library;
		Library.Load(Storage);
		if ( bInWon )
		{
			Session.Coins += ScaleCoins(WinCoinsPerLevel * (Campaign.Position + 1));
			Library.RecordWin(Campaign.Id, Campaign.Position, Session.Coins);
			Library.Save(Storage);
		}
		const model::CampaignDef* const Found = Library.FindCampaign(Campaign.Id);
		bCampaignHasNext = bInWon && Found && Campaign.Position + 1 < static_cast<int>(Found->LevelIds.Size());
	}
	// Coins and progress are earned only in the real levels.
	if ( !IsCustom() )
	{
		auto& CurrentProfile = GameContext::Get<ProfileManager>().Get();
		if ( bInWon )
		{
			Session.Coins += ScaleCoins(WinCoinsPerLevel * (LevelIndex + 1));
			CurrentProfile.LevelsCompleted = std::max(CurrentProfile.LevelsCompleted, LevelIndex + 1);
		}
		if ( bInWon )
		{
			CurrentProfile.LevelDeaths.Remove(LevelIndex);
		}
		else if ( CurrentDifficulty().bDeathPenalty )
		{
			++CurrentProfile.LevelDeaths[LevelIndex];
		}
		CurrentProfile.Coins += Session.Coins;
		CurrentProfile.BestScore = std::max(CurrentProfile.BestScore, Session.Score);
		int& ModeBest = CurrentProfile.BestScores[CurrentDifficulty().Id];
		ModeBest = std::max(ModeBest, Session.Score);
		AchievementManager& Achievements = GameContext::Get<AchievementManager>();
		for ( const auto& Count : { std::make_pair(counters::Kills, Session.Kills), std::make_pair(counters::LevelsWon, bInWon ? 1 : 0),
			std::make_pair(counters::PerfectLevels, bInWon && !bPlayerHurt ? 1 : 0), std::make_pair(counters::CoinsEarned, Session.Coins) } )
		{
			for ( const std::string& Id : Achievements.AddToCounter(Count.first, Count.second) )
			{
				const model::AchievementDef* const Reached = Achievements.GetBook().Find(Id);
				NewAchievements.PushBack(Text(Reached ? Reached->NameKey : Id));
			}
		}
		GameContext::Get<ProfileManager>().Save();
	}

	GameContext::Get<AudioManager>().Play(bInWon ? ids::sounds::LevelComplete : ids::sounds::GameOver);

	std::string Message = bInWon ? "" : Text(LoseReasonKey) + "\n";
	Message += Text("RESULT_KILLS") + ": " + std::to_string(Session.Defeated) + "/" + std::to_string(Session.TotalEnemies)
		+ "\n" + Text("RESULT_SCORE") + ": " + std::to_string(Session.Score);
	if ( !IsCustom() || (IsCampaignLevel() && bInWon) )
	{
		Message += "\n" + Text("RESULT_COINS") + ": " + std::to_string(Session.Coins);
	}
	for ( const std::string& Achievement : NewAchievements )
	{
		Message += "\n" + Text("RESULT_ACHIEVEMENT") + ": " + Achievement;
	}
	const int Deaths = IsCustom() ? 0 : GetLevelDeaths(LevelIndex);
	const model::DeathPenalty Penalty = LoadDeathPenalty();
	const int RepairCost = Penalty.GetRepairCost(Deaths);
	const bool bCanRepair = Deaths > 0 && GameContext::Get<ProfileManager>().Get().Coins >= RepairCost;
	if ( Deaths > 0 )
	{
		Message += "\n\n" + Text("RESULT_DAMAGED") + ": " + std::to_string(static_cast<int>(std::lround(Penalty.GetHealthShare(Deaths) * 100))) + "%"
			+ "\n" + Text("RESULT_REPAIR_COST") + ": " + std::to_string(RepairCost);
	}

	const bool bHasNext = IsCampaignLevel() ? bCampaignHasNext : (!IsCustom() && bInWon && LevelIndex + 1 < LevelLoader::GetBuiltinCount());
	if ( (!IsCustom() || IsCampaignLevel()) && bInWon && !bHasNext )
	{
		Message += "\n\n" + Text("RESULT_ALL_DONE");
	}

	TArray<widgets::Overlay::ButtonSpec> CurrentButtons;
	if ( bCanRepair )
	{
		CurrentButtons.PushBack({ Text("RESULT_REPAIR") + "  " + std::to_string(RepairCost), [this, RepairCost]()
		{
			auto& CurrentProfile = GameContext::Get<ProfileManager>().Get();
			CurrentProfile.Coins -= RepairCost;
			CurrentProfile.LevelDeaths.Remove(LevelIndex);
			GameContext::Get<ProfileManager>().Save();
			Restart();
		} });
	}
	if ( bHasNext )
	{
		const int CurrentNext = LevelIndex + 1;
		const std::string CampaignId = Campaign.Id;
		const int NextPosition = Campaign.Position + 1;
		CurrentButtons.PushBack({ Text("RESULT_NEXT"), [CurrentNext, CampaignId, NextPosition]()
		{
			SwitchScene(CampaignId.empty() ? GameScene::CreateScene(CurrentNext) : GameScene::CreateCampaignScene(CampaignId, NextPosition));
		} });
	}
	else
	{
		CurrentButtons.PushBack({ Text("RESULT_RETRY"), [this]() { Restart(); } });
	}
	if ( !IsCustom() )
	{
		CurrentButtons.PushBack({ Text("RESULT_UPGRADES"), []() { SwitchScene(UpgradesScene::CreateScene()); } });
	}
	CurrentButtons.PushBack({ Text(OpenedFrom == Origin::Editor ? "EDITOR_TITLE" : "PAUSE_MENU"), [this]() { Leave(); } });

	addChild(widgets::Overlay::create(Text(bInWon ? "RESULT_WIN" : "RESULT_LOSE"), Message, CurrentButtons), ZOverlay);
}

void GameScene::TogglePause()
{
	SetPaused(!bPaused);
}

void GameScene::SetPaused(const bool bInPaused)
{
	if ( bInPaused == bPaused )
	{
		return;
	}
	bPaused = bInPaused;
	GameContext::Get<AudioManager>().Play(ids::sounds::Pause);
	GameContext::Get<MusicManager>().SetPaused(bPaused);

	if ( bPaused )
	{
		ShowPauseMenu();
		cocos2d::Director::getInstance()->pause();
	}
	else
	{
		cocos2d::Director::getInstance()->resume();
		HidePauseLayers();
	}
}

void GameScene::HidePauseLayers()
{
	for ( cocos2d::Node** Layer : { &PauseOverlay, &OptionsOverlay } )
	{
		if ( *Layer )
		{
			GameContext::Get<HoverManager>().PopModal(*Layer);
			(*Layer)->removeFromParent();
			*Layer = nullptr;
		}
	}
}

void GameScene::ShowPauseMenu()
{
	HidePauseLayers();
	MenuBindings Bindings;
	Bindings.Conditions = {
		{ "from_editor", [this]() { return OpenedFrom == Origin::Editor; } },
		{ "from_menu", [this]() { return OpenedFrom != Origin::Editor; } }
	};
	Bindings.Actions = {
		{ "resume", [this]() { SetPaused(false); } },
		{ "options", [this]() { ShowOptions(); } },
		{ "restart", [this]() { Restart(); } },
		{ "leave", [this]() { Leave(); } }
	};
	Bindings.Slots["extra_pause"] = ExtraPauseButtons;
	PauseOverlay = widgets::Overlay::CreateFromMenu(ids::menus::Pause, Bindings);
	addChild(PauseOverlay, ZOverlay);
	GameContext::Get<HoverManager>().PushModal(PauseOverlay);
}

void GameScene::ShowOptions()
{
	HidePauseLayers();
	OptionsOverlay = widgets::Overlay::CreateEmpty();
	OptionsOverlay->addChild(SettingsPanel::create(SettingsPanel::Place::Level, [this]() { ShowPauseMenu(); }, [this]() { ShowOptions(); }));
	OptionsOverlay->addChild(widgets::CreateControlHints(true), 1);
	addChild(OptionsOverlay, ZOverlay);
	GameContext::Get<HoverManager>().PushModal(OptionsOverlay);
}

void GameScene::OnBackKey()
{
	if ( OptionsOverlay )
	{
		ShowPauseMenu();
		return;
	}
	TogglePause();
}

void GameScene::Restart()
{
	SetPaused(false);
	if ( IsCampaignLevel() )
	{
		SwitchScene(GameScene::CreateCampaignScene(Campaign.Id, Campaign.Position));
		return;
	}
	SwitchScene(IsCustom() ? GameScene::CreateCustomScene(OpenedFrom) : GameScene::CreateScene(LevelIndex));
}

void GameScene::Leave()
{
	SetPaused(false);
	SwitchScene(OpenedFrom == Origin::Editor ? EditorScene::CreateScene() : MenuScene::CreateScene());
}

} // namespace game::view::cocos
