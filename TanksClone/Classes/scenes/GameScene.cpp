#include "scenes/GameScene.h"

#include <algorithm>
#include <cmath>
#include <optional>

#include "core/BuildInfo.h"
#include "core/ConfigReader.h"
#include "core/DataConverter.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "game/ai/TankAi.h"
#include "game/objects/Bonus.h"
#include "game/objects/Tank.h"
#include "game/objects/Wall.h"
#include "game/ObjectsFactory.h"
#include "game/ResourceManifest.h"
#include "scenes/EditorScene.h"
#include "scenes/MenuScene.h"
#include "scenes/SceneUtils.h"
#include "scenes/UpgradesScene.h"
#include "model/DeathPenalty.h"
#include "services/AchievementManager.h"
#include "services/AudioManager.h"
#include "services/DifficultyManager.h"
#include "services/DisplaySettings.h"
#include "services/ProfileManager.h"
#include "services/ResourceManager.h"
#include "services/StringsManager.h"
#include "services/UpgradeManager.h"
#include "services/UiStyle.h"
#include "ui/ActionListener.h"
#include "ui/Background.h"
#include "ui/Hud.h"
#include "ui/SettingsPanel.h"
#include "ui/Widgets.h"

namespace tanks::view::cocos
{

namespace
{
	const model::DifficultyDef& CurrentDifficulty()
	{
		return GameContext::Get().GetManager<DifficultyManager>().GetCurrent();
	}

	std::string ConfigPath(const model::ResourceId& InConfig)
	{
		return GameContext::Get().GetManager<ResourceManager>().ResolvePath(InConfig, model::ResourceKind::Config);
	}

	constexpr int KillScore = 100;
	constexpr int KillCoins = 10;
	constexpr int WinCoinsPerLevel = 50;
	constexpr float ResultDelayWin = 1.0f;
	constexpr float ResultDelayLose = 1.4f;
	constexpr int ZAiDebug = 100;
	constexpr float AiDebugFontSize = 11.0f;
	constexpr float AiDebugPanelX = 0.01f;
	constexpr float AiDebugPanelY = 0.8f;
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

cocos2d::Scene* GameScene::CreateScene(const int InLevelIndex, ResourceHolder InPreloaded)
{
	return create(InLevelIndex, Origin::Menu, nullptr, true, &InPreloaded);
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

GameScene* GameScene::create(const int InLevelIndex, const Origin InOrigin, const LevelData* const InProvided, const bool bInWithBot, ResourceHolder* const InPreloaded)
{
	GameScene* NewGameScene = new (std::nothrow) GameScene();
	if ( NewGameScene && NewGameScene->init(InLevelIndex, InOrigin, InProvided, bInWithBot, InPreloaded) )
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

bool GameScene::init(const int InLevelIndex, const Origin InOrigin, const LevelData* const InProvided, const bool bInWithBot, ResourceHolder* const InPreloaded)
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
	if ( InPreloaded )
	{
		Resources.reset(new ResourceHolder(std::move(*InPreloaded)));
	}
	else
	{
		Resources.reset(new ResourceHolder());
		const ResourceSet Needed = Manifest.GetLevelResources(Level, GameContext::Get().GetManager<AudioManager>().IsEnabled());
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

	Playfield = cocos2d::Rect(0, 0, GridColumns * TileSize, GridRows * TileSize);
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
	auto PlayArea = Node::create();
	PlayArea->setPosition(0, BottomHudHeight);
	addChild(PlayArea, 1);
	PlayArea->addChild(CreateBackground(IsCustom() ? 7u : static_cast<unsigned>(InLevelIndex + 1), Tileset), ZBackground);

	World = Node::create();
	PlayArea->addChild(World, 1);

	addChild(Hud::create(&Session), ZHud);

	BuildLevel();
	SpawnPlayer(PlayerSpawn);
	if ( !bTestRun )
	{
		for ( Wave& CurrentWave : Level.Waves )
		{
			CurrentWave.Interval *= CurrentDifficulty().SpawnInterval;
		}
	}
	if ( const model::IConfigNode* const DropsNode = GameContext::Get().GetManager<ConfigReader>().GetConfig(ConfigPath(ids::configs::Drops)).FindChild("drops") )
	{
		Drops.Load(*DropsNode);
	}
	Spawner.reset(new EnemySpawner(World, Playfield, Level, EnemySpawns));
	Spawner->OnSpawned = [this](Tank& InEnemy) { SetUpEnemy(InEnemy); };
	if ( !Level.Script.IsEmpty() )
	{
		Script = std::make_unique<SceneScript>(Level.Script, [this](const std::string& InText) { ShowBanner(Text(InText)); });
	}
	Spawner->OnWaveStarted = [this](const int InWave, const int InWaveCount)
	{
		Session.WaveNumber = InWave;
		Session.WaveCount = InWaveCount;
		if ( InWaveCount > 1 )
		{
			ShowBanner(Text("WAVE_WORD") + " " + std::to_string(InWave) + "/" + std::to_string(InWaveCount));
		}
	};

	AddActionHandler(this, [this](const model::InputAction InAction)
	{
		if ( bFinished )
		{
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
		});
	}

	ShowBanner(Text(Level.Name));
	GameContext::Get().GetManager<AudioManager>().Play(ids::sounds::LevelStart);
	scheduleUpdate();
	return true;
}

void GameScene::onEnterTransitionDidFinish()
{
	TypedScene::onEnterTransitionDidFinish();
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
					SpawnWall(ids::configs::WallBrick, Position);
					break;
				case tiles::Stone:
					SpawnWall(ids::configs::WallStone, Position);
					break;
				case tiles::Wood:
					SpawnWall(ids::configs::WallWood, Position);
					break;
				case tiles::Base:
					if ( GameObject* const Base = SpawnWall(ids::configs::WallBase, Position) )
					{
						Base->SetOnKilledCallback([this](GameObject&) { Finish(false, ResultDelayLose); });
					}
					break;
				case tiles::Ammo:
					SpawnCrate(Position, ConfigPath(ids::configs::BonusAmmo));
					break;
				case tiles::Repair:
					SpawnCrate(Position, ConfigPath(ids::configs::BonusRepair));
					break;
				case tiles::Crate:
					SpawnCrate(Position, "");
					break;
				case tiles::FuelBarrel:
					GameContext::Get().GetManager<ObjectsFactory>().CreateFromConfig(ConfigPath(ids::configs::FuelBarrel), World, PositionOverride(Position));
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

void GameScene::SpawnObject(const model::ResourceId& InConfig, const cocos2d::Vec2& InPosition, int /*InZOrder*/)
{
	GameContext::Get().GetManager<ObjectsFactory>().CreateFromConfig(ConfigPath(InConfig), World, PositionOverride(InPosition));
}

GameObject* GameScene::SpawnWall(const model::ResourceId& InConfig, const cocos2d::Vec2& InPosition)
{
	const std::string Path = ConfigPath(InConfig);
	const cocos2d::ValueMap& ConfigData = GameContext::Get().GetManager<ConfigReader>().GetObjectData(Path).Values;
	cocos2d::ValueMap Overrides = PositionOverride(InPosition);
	Overrides["texture_path"] = ResourceManifest::GetTilesetFrame(Tileset, DataConverter::GetString(ConfigData, "texture_path", ""));
	return GameContext::Get().GetManager<ObjectsFactory>().CreateFromConfig(Path, World, Overrides);
}

namespace
{
	model::DeathPenalty LoadDeathPenalty()
	{
		model::DeathPenalty Penalty;
		const std::string Path = GameContext::Get().GetManager<ResourceManager>().ResolvePath(ids::configs::DeathPenalty, model::ResourceKind::Config);
		if ( const model::IConfigNode* const Node = GameContext::Get().GetManager<ConfigReader>().GetConfig(Path).FindChild("death_penalty") )
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
		const auto& Deaths = GameContext::Get().GetManager<ProfileManager>().Get().LevelDeaths;
		const auto Found = Deaths.find(InLevel);
		return Found != Deaths.end() ? Found->second : 0;
	}
}

void GameScene::SpawnPlayer(const cocos2d::Vec2& InPosition)
{
	auto Player = GameContext::Get().GetManager<ObjectsFactory>().CreateFromConfig<Tank>(ConfigPath(ids::configs::PlayerTank), World, PositionOverride(InPosition));
	if ( !Player )
	{
		CCLOGERROR("GameScene: could not create the player tank");
		return;
	}

	Player->SetBounds(Playfield);
	GameContext::Get().GetManager<UpgradeManager>().ApplyTo(*Player);
	if ( !IsCustom() )
	{
		const int Deaths = GetLevelDeaths(LevelIndex);
		const model::DeathPenalty Penalty = LoadDeathPenalty();
		Player->SetHealth(static_cast<int>(std::ceil(Player->GetMaxHealth() * Penalty.GetHealthShare(Deaths))));
		if ( !Player->HasUnlimitedAmmo() )
		{
			Player->SetAmmo(static_cast<int>(std::ceil(Player->GetMaxAmmo() * Penalty.GetAmmoShare(Deaths))));
		}
	}
	PlayerHealth = Player->GetHealth();
	Player->SetOnKilledCallback([this](GameObject&)
	{
		this->Player = nullptr;
		OnPlayerKilled();
	});
	this->Player = Player;
}

void GameScene::ShowBanner(const std::string& InText)
{
	const viewmodel::HudLayout& HudStyle = GameContext::Get().GetManager<UiStyle>().GetHud();
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
	}
	if ( Script )
	{
		Script->Update(InDelta);
	}
	UpdateAiDebug();
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
		GameContext::Get().GetManager<ObjectsFactory>().ForEachObject<Tank>([this, &Used, &Panel](Tank& InTank)
		{
			if ( InTank.IsDestroying() || !InTank.GetAi() || !InTank.GetVisualNode() )
			{
				return;
			}
			if ( Used == AiDebugLabels.size() )
			{
				cocos2d::Label* const NewLabel = widgets::CreateLabel("", AiDebugFontSize, widgets::colors::Accent(), false);
				AiDebugLayer->addChild(NewLabel);
				AiDebugLabels.push_back(NewLabel);
			}
			cocos2d::Label* const CurrentLabel = AiDebugLabels[Used++];
			const std::string Tag = "#" + std::to_string(Used) + " " + std::string(model::GetTacticName(InTank.GetAi()->GetTactic()));
			CurrentLabel->setString(Tag);
			Panel += Tag + "  " + InTank.GetName() + "  hp " + std::to_string(InTank.GetHealth()) + "/" + std::to_string(InTank.GetMaxHealth())
				+ "  arm " + std::to_string(InTank.GetArmor()) + "  dmg " + std::to_string(InTank.GetDamage())
				+ "  spd " + std::to_string(static_cast<int>(InTank.GetSpeed())) + "  phase " + std::to_string(InTank.GetPhase())
				+ (InTank.GetReloadRemaining() > 0.0f ? "  reload" : "");
			for ( const BuffSet::Active& CurrentBuff : InTank.GetBuffs().GetActive() )
			{
				Panel += "  +" + CurrentBuff.Def.Id;
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
	for ( size_t Index = Used; Index < AiDebugLabels.size(); ++Index )
	{
		AiDebugLabels[Index]->setVisible(false);
	}
	AiDebugPanel->setString(Panel);
	AiDebugPanel->setVisible(bShowAiDebug);
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
		Session.Coins += ScaleCoins(CurrentTank ? CurrentTank->GetCoinReward() : KillCoins);
	}

	if ( CurrentTank )
	{
		DropPickup(*CurrentTank);
		SpawnSplitParts(*CurrentTank);
	}

	if ( Session.Kills >= Session.TotalEnemies )
	{
		Finish(true, ResultDelayWin);
	}
}

void GameScene::SetUpEnemy(Tank& InEnemy)
{
	if ( !bTestRun )
	{
		InEnemy.ApplyDifficulty(CurrentDifficulty());
	}
	if ( InEnemy.GetAi() )
	{
		InEnemy.GetAi()->ApplyFeatures(Level.AiFeatures);
	}
	InEnemy.SetOnKilledCallback([this](GameObject& InObject) { OnEnemyKilled(InObject); });
	InEnemy.SetOnPhaseCallback([this](Tank& InBoss, const TankPhaseDef& InPhase)
	{
		if ( !bFinished )
		{
			SpawnBeside(InBoss, InPhase.SummonConfig, InPhase.SummonCount, InPhase.SummonSpacingTiles);
		}
	});
	if ( Script )
	{
		Script->OnEnemySpawned(InEnemy);
	}
}

void GameScene::SpawnSplitParts(const Tank& InEnemy)
{
	SpawnBeside(InEnemy, InEnemy.GetSplitInto(), InEnemy.GetSplitCount(), InEnemy.GetSplitSpacingTiles());
}

void GameScene::SpawnBeside(const Tank& InEnemy, const std::string& InConfig, const int InCount, const float InSpacingTiles)
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
		Tank* const Part = GameContext::Get().GetManager<ObjectsFactory>().CreateFromConfig<Tank>(InConfig, World, PositionOverride(Center + Side * Offset));
		if ( !Part )
		{
			continue;
		}
		Part->SetBounds(Playfield);
		Part->TurnTo(model::TurnState::Normalize(InEnemy.GetVisualNode()->getRotation()));
		SetUpEnemy(*Part);
		++Session.TotalEnemies;
	}
}

void GameScene::DropPickup(const Tank& InEnemy)
{
	const float ChanceScale = bTestRun ? 1.0f : CurrentDifficulty().Drops;
	const std::optional<std::string> Pickup = Drops.Roll(InEnemy.GetDropTable(), model::GameRandom(), ChanceScale);
	if ( !Pickup || !InEnemy.GetVisualNode() )
	{
		return;
	}
	++Session.Drops;
	SpawnPickup(*Pickup, InEnemy.GetVisualNode()->getPosition());
}

void GameScene::SpawnCrate(const cocos2d::Vec2& InPosition, const std::string& InContents)
{
	cocos2d::ValueMap Overrides = PositionOverride(InPosition);
	if ( !InContents.empty() )
	{
		Overrides["contents"] = InContents;
	}
	GameObject* const Crate = GameContext::Get().GetManager<ObjectsFactory>().CreateFromConfig(ConfigPath(ids::configs::Crate), World, Overrides);
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
		}
		if ( Pickup )
		{
			SpawnPickup(*Pickup, InPosition);
		}
	});
}

void GameScene::SpawnPickup(const std::string& InConfig, const cocos2d::Vec2& InPosition)
{
	cocos2d::ValueMap Overrides = PositionOverride(InPosition);
	Overrides["zorder"] = ZBonus;
	Bonus* const Dropped = GameContext::Get().GetManager<ObjectsFactory>().CreateFromConfig<Bonus>(InConfig, World, Overrides);
	if ( Dropped )
	{
		Dropped->SetOnPicked([this]() { ++Session.DropsPicked; });
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

	runAction(cocos2d::Sequence::create(
		cocos2d::DelayTime::create(InDelay),
		cocos2d::CallFunc::create([this, bInWon]() { ShowResult(bInWon); }),
		nullptr));
}

void GameScene::ShowResult(const bool bInWon)
{
	std::vector<std::string> NewAchievements;
	// Coins and progress are earned only in the real levels.
	if ( !IsCustom() )
	{
		auto& CurrentProfile = GameContext::Get().GetManager<ProfileManager>().Get();
		if ( bInWon )
		{
			Session.Coins += ScaleCoins(WinCoinsPerLevel * (LevelIndex + 1));
			CurrentProfile.LevelsCompleted = std::max(CurrentProfile.LevelsCompleted, LevelIndex + 1);
		}
		if ( bInWon )
		{
			CurrentProfile.LevelDeaths.erase(LevelIndex);
		}
		else if ( CurrentDifficulty().bDeathPenalty )
		{
			++CurrentProfile.LevelDeaths[LevelIndex];
		}
		CurrentProfile.Coins += Session.Coins;
		CurrentProfile.BestScore = std::max(CurrentProfile.BestScore, Session.Score);
		int& ModeBest = CurrentProfile.BestScores[CurrentDifficulty().Id];
		ModeBest = std::max(ModeBest, Session.Score);
		AchievementManager& Achievements = GameContext::Get().GetManager<AchievementManager>();
		for ( const auto& Count : { std::make_pair(counters::Kills, Session.Kills), std::make_pair(counters::LevelsWon, bInWon ? 1 : 0),
			std::make_pair(counters::PerfectLevels, bInWon && !bPlayerHurt ? 1 : 0), std::make_pair(counters::CoinsEarned, Session.Coins) } )
		{
			for ( const std::string& Id : Achievements.AddToCounter(Count.first, Count.second) )
			{
				const model::AchievementDef* const Reached = Achievements.GetBook().Find(Id);
				NewAchievements.push_back(Text(Reached ? Reached->NameKey : Id));
			}
		}
		GameContext::Get().GetManager<ProfileManager>().Save();
	}

	GameContext::Get().GetManager<AudioManager>().Play(bInWon ? ids::sounds::LevelComplete : ids::sounds::GameOver);

	std::string Message = Text("RESULT_KILLS") + ": " + std::to_string(Session.Kills) + "/" + std::to_string(Session.TotalEnemies)
		+ "\n" + Text("RESULT_SCORE") + ": " + std::to_string(Session.Score);
	if ( !IsCustom() )
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
	const bool bCanRepair = Deaths > 0 && GameContext::Get().GetManager<ProfileManager>().Get().Coins >= RepairCost;
	if ( Deaths > 0 )
	{
		Message += "\n\n" + Text("RESULT_DAMAGED") + ": " + std::to_string(static_cast<int>(std::lround(Penalty.GetHealthShare(Deaths) * 100))) + "%"
			+ "\n" + Text("RESULT_REPAIR_COST") + ": " + std::to_string(RepairCost);
	}

	const bool bHasNext = !IsCustom() && bInWon && LevelIndex + 1 < LevelLoader::GetBuiltinCount();
	if ( !IsCustom() && bInWon && !bHasNext )
	{
		Message += "\n\n" + Text("RESULT_ALL_DONE");
	}

	std::vector<widgets::Overlay::ButtonSpec> CurrentButtons;
	if ( bCanRepair )
	{
		CurrentButtons.push_back({ Text("RESULT_REPAIR") + "  " + std::to_string(RepairCost), [this, RepairCost]()
		{
			auto& CurrentProfile = GameContext::Get().GetManager<ProfileManager>().Get();
			CurrentProfile.Coins -= RepairCost;
			CurrentProfile.LevelDeaths.erase(LevelIndex);
			GameContext::Get().GetManager<ProfileManager>().Save();
			Restart();
		} });
	}
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
	GameContext::Get().GetManager<AudioManager>().Play(ids::sounds::Pause);

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
			(*Layer)->removeFromParent();
			*Layer = nullptr;
		}
	}
}

void GameScene::ShowPauseMenu()
{
	HidePauseLayers();
	PauseOverlay = widgets::Overlay::create(Text("PAUSE_TITLE"), "", {
		{ Text("PAUSE_RESUME"), [this]() { SetPaused(false); } },
		{ Text("PAUSE_OPTIONS"), [this]() { ShowOptions(); } },
		{ Text("PAUSE_RESTART"), [this]() { Restart(); } },
		{ Text(OpenedFrom == Origin::Editor ? "EDITOR_TITLE" : "PAUSE_MENU"), [this]() { Leave(); } }
	});
	addChild(PauseOverlay, ZOverlay);
}

void GameScene::ShowOptions()
{
	HidePauseLayers();
	OptionsOverlay = widgets::Overlay::CreateEmpty();
	OptionsOverlay->addChild(SettingsPanel::create(SettingsPanel::Place::Level, [this]() { ShowPauseMenu(); }, [this]() { ShowOptions(); }));
	addChild(OptionsOverlay, ZOverlay);
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
	SwitchScene(IsCustom() ? GameScene::CreateCustomScene(OpenedFrom) : GameScene::CreateScene(LevelIndex));
}

void GameScene::Leave()
{
	SetPaused(false);
	SwitchScene(OpenedFrom == Origin::Editor ? EditorScene::CreateScene() : MenuScene::CreateScene());
}

} // namespace tanks::view::cocos
