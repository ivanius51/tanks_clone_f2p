#include "scenes/LoadingScene.h"

#include <algorithm>

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/GameData.h"
#include "game/components/Buff.h"
#include "game/EnemyCatalog.h"
#include "game/LevelLoader.h"
#include "game/ResourceManifest.h"
#include "model/Log.h"
#include "scenes/GameScene.h"
#include "scenes/MenuScene.h"
#include "scenes/SceneUtils.h"
#include "services/AudioManager.h"
#include "services/ProfileManager.h"
#include "services/StringsManager.h"
#include "services/UpgradeManager.h"
#include "services/UiStyle.h"
#include "ui/Widgets.h"

namespace game::view::cocos
{

namespace
{
	const viewmodel::LoadingLayout& Layout()
	{
		return GameContext::Get<UiStyle>().GetMenu().Loading;
	}

	constexpr int LogSteps = 10;
}

cocos2d::Scene* LoadingScene::CreateScene()
{
	return LoadingScene::create();
}

cocos2d::Scene* LoadingScene::CreateForLevel(const int InLevelIndex, const float InMinSeconds)
{
	LoadingScene* const Result = LoadingScene::create();
	if ( Result )
	{
		Result->LevelIndex = InLevelIndex;
		Result->MinSeconds = InMinSeconds;
	}
	return Result;
}

bool LoadingScene::init()
{
	if ( !Scene::init() )
	{
		return false;
	}

	addChild(widgets::CreateScreenBackground());

	auto Title = widgets::CreateLabel("TANKS CLONE", Layout().TitleFontSize, widgets::colors::Accent(), true);
	Title->setPosition(DesignWidth / 2, DesignHeight / 2 + Layout().TitleOffsetY);
	addChild(Title);

	auto Status = widgets::CreateLabel("Loading...", Layout().StatusFontSize, widgets::colors::TextDim());
	Status->setPosition(DesignWidth / 2, DesignHeight / 2 + Layout().StatusOffsetY);
	addChild(Status);
	return true;
}

void LoadingScene::onEnter()
{
	Scene::onEnter();
	if ( LevelIndex >= 0 )
	{
		return;
	}

	// Let the first frame appear before the (short) loading blocks the thread.
	runAction(cocos2d::Sequence::create(
		cocos2d::DelayTime::create(Layout().StartDelay),
		cocos2d::CallFunc::create([this]() { LoadResources(); }),
		cocos2d::DelayTime::create(Layout().MinShowTime),
		cocos2d::CallFunc::create([]() { SwitchScene(MenuScene::CreateScene()); }),
		nullptr));
}

void LoadingScene::onEnterTransitionDidFinish()
{
	TypedScene::onEnterTransitionDidFinish();
	if ( LevelIndex >= 0 )
	{
		// A scene switch during the fade-in would leave the new level half entered.
		StartLevelLoad();
	}
}

void LoadingScene::LoadResources()
{
	LoadGameData(false);
}

void LoadingScene::StartLevelLoad()
{
	LevelData Level;
	if ( !LevelLoader::Load(LevelLoader::BuiltinId(LevelIndex), Level) )
	{
		SwitchScene(GameScene::CreateScene(LevelIndex));
		return;
	}
	const bool bWithSounds = GameContext::Get<AudioManager>().IsEnabled();
	Job.Reset(new ResourceLoadJob(ResourceManifest().GetLevelResources(Level, bWithSounds), true));

	Bar = cocos2d::DrawNode::create();
	Bar->setPosition(DesignWidth / 2, DesignHeight / 2 + Layout().BarOffsetY);
	addChild(Bar);
	DrawProgress(0.0f);
	scheduleUpdate();
}

void LoadingScene::update(const float InDelta)
{
	Scene::update(InDelta);
	if ( !Job )
	{
		return;
	}
	ElapsedSeconds += InDelta;
	Job->Step();

	const float Real = Job->GetProgress();
	const float Shown = MinSeconds > 0.0f ? std::min(Real, ElapsedSeconds / MinSeconds) : Real;
	DrawProgress(Shown);
	const int Step = static_cast<int>(Shown * LogSteps);
	if ( Step != LoggedStep )
	{
		LoggedStep = Step;
		model::Log::Info("LoadingScene: level %d, %d%% after %.2f s", LevelIndex + 1, Step * (100 / LogSteps), ElapsedSeconds);
	}

	if ( Job->IsFinished() && ElapsedSeconds >= MinSeconds )
	{
		model::Log::Info("LoadingScene: level %d loaded in %.2f s, %d failed", LevelIndex + 1, ElapsedSeconds, static_cast<int>(Job->GetFailedCount()));
		unscheduleUpdate();
		const int Level = LevelIndex;
		SwitchScene(GameScene::CreateScene(Level, Job->TakeResources()));
		Job.Reset();
	}
}

void LoadingScene::DrawProgress(const float InProgress)
{
	Bar->clear();
	const cocos2d::Vec2 Low(-Layout().BarWidth / 2, -Layout().BarHeight / 2);
	const cocos2d::Vec2 High(Layout().BarWidth / 2, Layout().BarHeight / 2);
	Bar->drawSolidRect(Low, High, cocos2d::Color4F(widgets::colors::TextDim()));
	Bar->drawSolidRect(Low + cocos2d::Vec2(Layout().BarBorder, Layout().BarBorder), High - cocos2d::Vec2(Layout().BarBorder, Layout().BarBorder), cocos2d::Color4F::BLACK);
	const float FillWidth = (Layout().BarWidth - Layout().BarBorder * 2) * InProgress;
	Bar->drawSolidRect(Low + cocos2d::Vec2(Layout().BarBorder, Layout().BarBorder), Low + cocos2d::Vec2(Layout().BarBorder + FillWidth, Layout().BarHeight - Layout().BarBorder), cocos2d::Color4F(widgets::colors::Accent()));
}

} // namespace game::view::cocos
