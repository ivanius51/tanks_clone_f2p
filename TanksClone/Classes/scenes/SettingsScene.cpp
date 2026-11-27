#include "scenes/SettingsScene.h"

#include "core/ResourceIds.h"
#include "scenes/MenuScene.h"
#include "scenes/SceneUtils.h"
#include "ui/SettingsPanel.h"
#include "ui/WidgetFactory.h"
#include "ui/Widgets.h"

namespace game::view::cocos
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
	addChild(WidgetFactory::Create(ids::widgets::Backdrop));
	addChild(widgets::CreateControlHints(), 50);
	const auto Back = []() { SwitchScene(MenuScene::CreateScene()); };
	addChild(SettingsPanel::create(SettingsPanel::Place::Screen, Back, []() { SwitchScene(SettingsScene::CreateScene()); }), 1);
	widgets::AddBackKeyHandler(this, Back);
	return true;
}

} // namespace game::view::cocos
