#pragma once

#include "cocos2d.h"
#include "scenes/TypedScene.h"

namespace tanks::view::cocos
{

// The shop where coins are spent on upgrades for the player's tank.
class UpgradesScene : public TypedScene<SceneKind::Upgrades>
{
public:
	static cocos2d::Scene* CreateScene();

	bool init() override;

	CREATE_FUNC(UpgradesScene);

private:
	void Build(int InSelectedButton);

	int Selected = 0;
};

} // namespace tanks::view::cocos