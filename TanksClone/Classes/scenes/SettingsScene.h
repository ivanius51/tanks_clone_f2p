#pragma once

#include "cocos2d.h"
#include "scenes/TypedScene.h"

namespace tanks::view::cocos
{

class SettingsScene : public TypedScene<SceneKind::Settings>
{
public:
	static cocos2d::Scene* CreateScene();

	bool init() override;

	CREATE_FUNC(SettingsScene);

private:
	void Build();
};

} // namespace tanks::view::cocos