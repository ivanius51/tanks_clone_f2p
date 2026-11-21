#pragma once

#include "cocos2d.h"

#include "scenes/TypedScene.h"

namespace tanks::view::cocos
{

/** The settings screen of the menu: the shared settings panel over the menu background. */
class SettingsScene : public TypedScene<SceneKind::Settings>
{
public:
	static cocos2d::Scene* CreateScene();

	bool init() override;

	CREATE_FUNC(SettingsScene);
};

} // namespace tanks::view::cocos
