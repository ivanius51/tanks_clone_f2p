#include "scenes/LoadingScene.h"

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/GameData.h"
#include "game/components/Buff.h"
#include "game/EnemyCatalog.h"
#include "game/LevelLoader.h"
#include "game/ResourceManifest.h"
#include "scenes/GameScene.h"
#include "scenes/MenuScene.h"
#include "scenes/SceneUtils.h"
#include "services/AudioManager.h"
#include "services/ProfileManager.h"
#include "services/StringsManager.h"
#include "services/UpgradeManager.h"
#include "ui/Widgets.h"

namespace tanks::view::cocos
{

namespace
{
	constexpr float TitleFontSize = 84.0f;
	constexpr float StatusFontSize = 24.0f;
	constexpr float MinShowTime = 0.6f;
	constexpr float BarWidth = 480.0f;
	constexpr float BarHeight = 16.0f;
	constexpr float BarBorder = 2.0f;
}

cocos2d::Scene* LoadingScene::CreateScene()
{
	return LoadingScene::create();
}

cocos2d::Scene* LoadingScene::CreateForLevel(const int InLevelIndex)
{
	LoadingScene* const Result = LoadingScene::create();
	if ( Result )
	{
		Result->LevelIndex = InLevelIndex;
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

	auto Title = widgets::CreateLabel("TANKS CLONE", TitleFontSize, widgets::colors::Accent, true);
	Title->setPosition(DesignWidth / 2, DesignHeight / 2 + 40);
	addChild(Title);

	auto Status = widgets::CreateLabel("Loading...", StatusFontSize, widgets::colors::TextDim);
	Status->setPosition(DesignWidth / 2, DesignHeight / 2 - 50);
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
		cocos2d::DelayTime::create(0.05f),
		cocos2d::CallFunc::create([this]() { LoadResources(); }),
		cocos2d::DelayTime::create(MinShowTime),
		cocos2d::CallFunc::create([]() { SwitchScene(MenuScene::CreateScene()); }),
		nullptr));
}

void LoadingScene::onEnterTransitionDidFinish()
{
	Scene::onEnterTransitionDidFinish();
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
	const bool bWithSounds = GameContext::Get().GetManager<AudioManager>().IsEnabled();
	Job.reset(new ResourceLoadJob(ResourceManifest().GetLevelResources(Level, bWithSounds), true));

	Bar = cocos2d::DrawNode::create();
	Bar->setPosition(DesignWidth / 2, DesignHeight / 2 - 100);
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
	Job->Step();
	DrawProgress(Job->GetProgress());
	if ( Job->IsFinished() )
	{
		unscheduleUpdate();
		const int Level = LevelIndex;
		SwitchScene(GameScene::CreateScene(Level, Job->TakeResources()));
		Job.reset();
	}
}

void LoadingScene::DrawProgress(const float InProgress)
{
	Bar->clear();
	const cocos2d::Vec2 Low(-BarWidth / 2, -BarHeight / 2);
	const cocos2d::Vec2 High(BarWidth / 2, BarHeight / 2);
	Bar->drawSolidRect(Low, High, cocos2d::Color4F(widgets::colors::TextDim));
	Bar->drawSolidRect(Low + cocos2d::Vec2(BarBorder, BarBorder), High - cocos2d::Vec2(BarBorder, BarBorder), cocos2d::Color4F::BLACK);
	const float FillWidth = (BarWidth - BarBorder * 2) * InProgress;
	Bar->drawSolidRect(Low + cocos2d::Vec2(BarBorder, BarBorder), Low + cocos2d::Vec2(BarBorder + FillWidth, BarHeight - BarBorder), cocos2d::Color4F(widgets::colors::Accent));
}

} // namespace tanks::view::cocos
