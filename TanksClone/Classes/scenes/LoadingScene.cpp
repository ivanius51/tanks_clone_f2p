#include "scenes/LoadingScene.h"

#include "services/AudioManager.h"
#include "game/components/Buff.h"
#include "core/ConfigReader.h"
#include "game/EnemyCatalog.h"
#include "core/GameConstants.h"
#include "core/GameData.h"
#include "scenes/MenuScene.h"
#include "services/ProfileManager.h"
#include "scenes/SceneUtils.h"
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
}

cocos2d::Scene* LoadingScene::CreateScene()
{
	return LoadingScene::create();
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

	// Let the first frame appear before the (short) loading blocks the thread.
	runAction(cocos2d::Sequence::create(
		cocos2d::DelayTime::create(0.05f),
		cocos2d::CallFunc::create([this]() { LoadResources(); }),
		cocos2d::DelayTime::create(MinShowTime),
		cocos2d::CallFunc::create([]() { SwitchScene(MenuScene::CreateScene()); }),
		nullptr));
	return true;
}

void LoadingScene::LoadResources()
{
	LoadGameData(false);
}

} // namespace tanks::view::cocos