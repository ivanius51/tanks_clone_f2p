#include "scenes/GameScene.h"

#include <algorithm>

#include "core/ConfigReader.h"
#include "core/DataConverter.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/objects/Tank.h"
#include "game/ObjectsFactory.h"
#include "game/ResourceManifest.h"
#include "scenes/EditorScene.h"
#include "scenes/MenuScene.h"
#include "scenes/SceneUtils.h"
#include "scenes/UpgradesScene.h"
#include "services/AudioManager.h"
#include "services/DisplaySettings.h"
#include "services/ProfileManager.h"
#include "services/StringsManager.h"
#include "services/UpgradeManager.h"
#include "ui/Background.h"
#include "ui/Hud.h"
#include "ui/Widgets.h"

namespace tanks::view::cocos
{

namespace
{
	constexpr int KillScore = 100;
	constexpr int KillCoins = 10;
	constexpr int WinCoinsPerLevel = 50;
	constexpr float ResultDelayWin = 1.0f;
	constexpr float ResultDelayLose = 1.4f;
	constexpr float BannerFontSize = 48.0f;
	constexpr int ZHud = 50;
	constexpr int ZOverlay = 100;

	std::string Text(const std::string& InId)
	{
		return GameContext::Get().GetManager<StringsManager>().GetString(InId);
	}

	cocos2d::ValueMap PositionOverride(const cocos2d::Vec2& InPosition)
	{
		cocos2d::ValueVector Position;
		Position.push_back(cocos2d::Value(InPosition.x));
		Position.push_back(cocos2d::Value(InPosition.y));

		cocos2d::ValueMap Overrides;
		Overrides["position"] = Position;
		return Overrides;
	}
}

cocos2d::Scene* GameScene::CreateScene(const int InLevelIndex)
{
	return create(InLevelIndex, Origin::Menu);
}

cocos2d::Scene* GameScene::CreateCustomScene(const Origin InOrigin)
{
	return create(CustomLevel, InOrigin);
}

cocos2d::Scene* GameScene::CreateForTest(const LevelData& InLevel, const bool bInWithBot)
{
	return create(CustomLevel, Origin::Menu, &InLevel, bInWithBot);
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
	return Spawner.get();
}

const PlayerBot* GameScene::GetBot() const
{
	return Bot.get();
}

GameScene* GameScene::create(const int InLevelIndex, const Origin InOrigin, const LevelData* const InProvided, const bool bInWithBot)
{
	GameScene* NewGameScene = new (std::nothrow) GameScene();
	if ( NewGameScene && NewGameScene->init(InLevelIndex, InOrigin, InProvided, bInWithBot) )
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

bool GameScene::init(const int InLevelIndex, const Origin InOrigin, const LevelData* const InProvided, const bool bInWithBot)
{
	if ( !Scene::init() )
	{
		return false;
	}

	LevelIndex = InLevelIndex;
	OpenedFrom = InOrigin;

	bool CurrentbLoaded = false;
	if ( InProvided )
	{
		Level = *InProvided;
		CurrentbLoaded = LevelLoader::Validate(Level).empty();
		if ( Level.Waves.empty() )
		{
			// Same as the loader does for the simple form of a level.
			Wave CurrentWave;
			CurrentWave.MaxAlive = Level.MaxAlive;
			CurrentWave.Interval = Level.SpawnInterval;
			CurrentWave.Entries.push_back({ tiles::EnemySpawn, Level.TotalEnemies });
			Level.Waves.push_back(CurrentWave);
		}
		bTestRun = true;
		if ( bInWithBot )
		{
			Bot.reset(new PlayerBot());
		}
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
	Atlases.reset(new AtlasScope(Manifest.GetLevelAtlases(Level)));
	Tileset = Manifest.GetTilesetName(Level.Tileset);

	Playfield = cocos2d::Rect(0, 0, GridColumns * TileSize, GridRows * TileSize);
	Session.TotalEnemies = LevelLoader::GetTotalEnemies(Level);

	Session.LevelName = Text(Level.Name);

	addChild(widgets::CreateScreenBackground(), -1);

	// The playfield sits above the player's panel; everything in it uses its own coordinates.
	auto PlayArea = Node::create();
	PlayArea->setPosition(0, BottomHudHeight);
	addChild(PlayArea, 1);
	PlayArea->addChild(CreateBackground(IsCustom() ? 7u : static_cast<unsigned>(InLevelIndex + 1), Tileset), ZBackground);

	World = Node::create();
	PlayArea->addChild(World, 1);

	addChild(Hud::create(&Session), ZHud);

	BuildLevel();
	SpawnPlayer(PlayerSpawn);
	Spawner.reset(new EnemySpawner(World, Playfield, Level, EnemySpawns));
	Spawner->OnSpawned = [this](Tank& InEnemy)
	{
		InEnemy.SetOnKilledCallback([this](GameObject& InObject) { OnEnemyKilled(InObject); });
	};
	Spawner->OnWaveStarted = [this](const int InWave, const int InWaveCount)
	{
		Session.WaveNumber = InWave;
		Session.WaveCount = InWaveCount;
		if ( InWaveCount > 1 )
		{
			ShowBanner(Text("WAVE_WORD") + " " + std::to_string(InWave) + "/" + std::to_string(InWaveCount));
		}
	};

	auto Keyboard = cocos2d::EventListenerKeyboard::create();
	Keyboard->onKeyPressed = [this](const cocos2d::EventKeyboard::KeyCode InKeyCode, cocos2d::Event*)
	{
		if ( InKeyCode == cocos2d::EventKeyboard::KeyCode::KEY_ESCAPE && !bFinished )
		{
			TogglePause();
		}
	};
	_eventDispatcher->addEventListenerWithSceneGraphPriority(Keyboard, this);

	ShowBanner(Text(Level.Name));
	GameContext::Get().GetManager<AudioManager>().Play(Sound::LevelStart);
	scheduleUpdate();
	return true;
}

void GameScene::onEnterTransitionDidFinish()
{
	Scene::onEnterTransitionDidFinish();
	// After the transition: the previous level's onExit (which restores the menu rate) has already run.
	ApplyFpsLimit(GameContext::Get().GetManager<ProfileManager>().Get().FpsLimit);
}

void GameScene::onExit()
{
	if ( bPaused )
	{
		cocos2d::Director::getInstance()->resume();
		bPaused = false;
	}
	ApplyMenuFrameRate();
	Scene::onExit();
}

void GameScene::BuildLevel()
{
	for ( int Row = 0; Row < GridRows; ++Row )
	{
		const float Y = (GridRows - Row - 0.5f) * TileSize;
		for ( int Column = 0; Column < GridColumns; ++Column )
		{
			const cocos2d::Vec2 Position((Column + 0.5f) * TileSize, Y);
			switch ( Level.Rows[Row][Column] )
			{
				case tiles::Brick:
					SpawnWall("configs/wall_brick.json", Position);
					break;
				case tiles::Stone:
					SpawnWall("configs/wall_stone.json", Position);
					break;
				case tiles::Wood:
					SpawnWall("configs/wall_wood.json", Position);
					break;
				case tiles::Ammo:
					SpawnObject(BonusAmmoConfig, Position, ZBonus);
					break;
				case tiles::Repair:
					SpawnObject(BonusRepairConfig, Position, ZBonus);
					break;
				case tiles::PlayerSpawn:
					PlayerSpawn = Position;
					break;
				default:
					if ( tiles::IsSpawner(Level.Rows[Row][Column]) )
					{
						EnemySpawns[Level.Rows[Row][Column]].push_back(Position);
					}
					break;
			}
		}
	}
}

void GameScene::SpawnObject(const char* const InConfig, const cocos2d::Vec2& InPosition, int /*InZOrder*/)
{
	GameContext::Get().GetManager<ObjectsFactory>().CreateFromConfig(InConfig, World, PositionOverride(InPosition));
}

void GameScene::SpawnWall(const char* const InConfig, const cocos2d::Vec2& InPosition)
{
	const cocos2d::ValueMap& ConfigData = GameContext::Get().GetManager<ConfigReader>().GetObjectData(InConfig).Values;
	cocos2d::ValueMap Overrides = PositionOverride(InPosition);
	Overrides["texture_path"] = ResourceManifest::GetTilesetFrame(Tileset, DataConverter::GetString(ConfigData, "texture_path", ""));
	GameContext::Get().GetManager<ObjectsFactory>().CreateFromConfig(InConfig, World, Overrides);
}

void GameScene::SpawnPlayer(const cocos2d::Vec2& InPosition)
{
	auto Player = GameContext::Get().GetManager<ObjectsFactory>().CreateFromConfig<Tank>(PlayerTankConfig, World, PositionOverride(InPosition));
	if ( !Player )
	{
		CCLOGERROR("GameScene: could not create the player tank");
		return;
	}

	Player->SetBounds(Playfield);
	GameContext::Get().GetManager<UpgradeManager>().ApplyTo(*Player);
	Player->SetOnKilledCallback([this](GameObject&) { OnPlayerKilled(); });
}

void GameScene::ShowBanner(const std::string& InText)
{
	auto Banner = widgets::CreateLabel(InText, BannerFontSize, widgets::colors::Accent, true);
	Banner->setPosition(Playfield.getMidX(), BottomHudHeight + Playfield.getMidY() + TileSize * 2);
	Banner->setOpacity(0);
	addChild(Banner, ZOverlay - 1);
	Banner->runAction(cocos2d::Sequence::create(
		cocos2d::FadeIn::create(0.3f), cocos2d::DelayTime::create(1.4f), cocos2d::FadeOut::create(0.5f), cocos2d::RemoveSelf::create(), nullptr));
}

void GameScene::update(const float InDelta)
{
	if ( bFinished )
	{
		return;
	}

	Elapsed += InDelta;
	if ( Bot )
	{
		Bot->Update(InDelta);
	}
	if ( Spawner )
	{
		Spawner->Update(InDelta);
	}
}

void GameScene::OnEnemyKilled(GameObject& InEnemy)
{
	if ( bFinished )
	{
		return;
	}

	++Session.Kills;
	auto CurrentTank = ObjectCast<Tank>(&InEnemy);
	Session.Score += CurrentTank ? CurrentTank->GetScoreReward() : KillScore;
	if ( !IsCustom() )
	{
		Session.Coins += CurrentTank ? CurrentTank->GetCoinReward() : KillCoins;
	}

	if ( Session.Kills >= Session.TotalEnemies )
	{
		Finish(true, ResultDelayWin);
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

	runAction(cocos2d::Sequence::create(
		cocos2d::DelayTime::create(InDelay),
		cocos2d::CallFunc::create([this, bInWon]() { ShowResult(bInWon); }),
		nullptr));
}

void GameScene::ShowResult(const bool bInWon)
{
	// Coins and progress are earned only in the real levels.
	if ( !IsCustom() )
	{
		auto& CurrentProfile = GameContext::Get().GetManager<ProfileManager>().Get();
		if ( bInWon )
		{
			Session.Coins += WinCoinsPerLevel * (LevelIndex + 1);
			CurrentProfile.LevelsCompleted = std::max(CurrentProfile.LevelsCompleted, LevelIndex + 1);
		}
		CurrentProfile.Coins += Session.Coins;
		CurrentProfile.BestScore = std::max(CurrentProfile.BestScore, Session.Score);
		GameContext::Get().GetManager<ProfileManager>().Save();
	}

	GameContext::Get().GetManager<AudioManager>().Play(bInWon ? Sound::LevelComplete : Sound::GameOver);

	std::string Message = Text("RESULT_KILLS") + ": " + std::to_string(Session.Kills) + "/" + std::to_string(Session.TotalEnemies)
		+ "\n" + Text("RESULT_SCORE") + ": " + std::to_string(Session.Score);
	if ( !IsCustom() )
	{
		Message += "\n" + Text("RESULT_COINS") + ": " + std::to_string(Session.Coins);
	}

	const bool bHasNext = !IsCustom() && bInWon && LevelIndex + 1 < LevelLoader::GetBuiltinCount();
	if ( !IsCustom() && bInWon && !bHasNext )
	{
		Message += "\n\n" + Text("RESULT_ALL_DONE");
	}

	std::vector<widgets::Overlay::ButtonSpec> CurrentButtons;
	if ( bHasNext )
	{
		const int CurrentNext = LevelIndex + 1;
		CurrentButtons.push_back({ Text("RESULT_NEXT"), [CurrentNext]() { SwitchScene(GameScene::CreateScene(CurrentNext)); } });
	}
	else
	{
		CurrentButtons.push_back({ Text("RESULT_RETRY"), [this]() { Restart(); } });
	}
	if ( !IsCustom() )
	{
		CurrentButtons.push_back({ Text("RESULT_UPGRADES"), []() { SwitchScene(UpgradesScene::CreateScene()); } });
	}
	CurrentButtons.push_back({ Text(OpenedFrom == Origin::Editor ? "EDITOR_TITLE" : "PAUSE_MENU"), [this]() { Leave(); } });

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
	GameContext::Get().GetManager<AudioManager>().Play(Sound::Pause);

	if ( bPaused )
	{
		PauseOverlay = widgets::Overlay::create(Text("PAUSE_TITLE"), "", {
			{ Text("PAUSE_RESUME"), [this]() { SetPaused(false); } },
			{ Text("PAUSE_RESTART"), [this]() { Restart(); } },
			{ Text(OpenedFrom == Origin::Editor ? "EDITOR_TITLE" : "PAUSE_MENU"), [this]() { Leave(); } }
		});
		addChild(PauseOverlay, ZOverlay);
		cocos2d::Director::getInstance()->pause();
	}
	else
	{
		cocos2d::Director::getInstance()->resume();
		if ( PauseOverlay )
		{
			PauseOverlay->removeFromParent();
			PauseOverlay = nullptr;
		}
	}
}

void GameScene::Restart()
{
	SetPaused(false);
	SwitchScene(IsCustom() ? GameScene::CreateCustomScene(OpenedFrom) : GameScene::CreateScene(LevelIndex));
}

void GameScene::Leave()
{
	SetPaused(false);
	SwitchScene(OpenedFrom == Origin::Editor ? EditorScene::CreateScene() : MenuScene::CreateScene());
}

} // namespace tanks::view::cocos
