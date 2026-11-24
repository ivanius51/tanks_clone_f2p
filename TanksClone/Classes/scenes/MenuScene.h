#pragma once

#include "cocos2d.h"

#include "scenes/TypedScene.h"

namespace tanks::view::cocos
{
class LayoutWidget;

class MenuScene : public TypedScene<SceneKind::Menu>
{
public:
	static cocos2d::Scene* CreateScene();

	bool init() override;

	CREATE_FUNC(MenuScene);

private:
	void StartNewGame();
	void ContinueGame();
	void AskNewGame();
	void ShowDebugMenu();
	void ShowGalleries();
	void FitGroup();

	LayoutWidget* Group = nullptr;
};
} // namespace tanks::view::cocos
