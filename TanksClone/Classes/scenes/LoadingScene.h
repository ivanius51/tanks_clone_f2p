#pragma once

#include "cocos2d.h"

#include "scenes/TypedScene.h"

namespace tanks::view::cocos
{

// Shown at startup while the sprites, configs, profile and sounds are loaded.
class LoadingScene : public TypedScene<SceneKind::Loading>
{
public:
	static cocos2d::Scene* CreateScene();

	bool init() override;

	CREATE_FUNC(LoadingScene);

private:
	void LoadResources();
};

} // namespace tanks::view::cocos
