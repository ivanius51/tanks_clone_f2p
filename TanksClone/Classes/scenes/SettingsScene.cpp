#include "scenes/SettingsScene.h"

#include "scenes/MenuScene.h"
#include "scenes/SceneUtils.h"
#include "ui/SettingsPanel.h"
#include "ui/Widgets.h"

namespace tanks::view::cocos
{

cocos2d::Scene* SettingsScene::CreateScene()
{
	return SettingsScene::create();
}

bool SettingsScene::init()
{
	if ( !Scene::init() )
	{
		return false;
	}
	addChild(widgets::CreateScreenBackground());
	addChild(SettingsPanel::create(SettingsPanel::Place::Screen, []() { SwitchScene(MenuScene::CreateScene()); }), 1);
	return true;
}

} // namespace tanks::view::cocos
