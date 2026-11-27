#pragma once

#include "cocos2d.h"

#include "scenes/TypedScene.h"
#include "templates/TContainers.h"
#include "ui/Widgets.h"

namespace game::view::cocos
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
	void ShowStatus();
	void ShowCredits();
	void ShowCustomMenu(const TArray<widgets::Overlay::ButtonSpec>& InCampaignButtons);

	LayoutWidget* Group = nullptr;
};
} // namespace game::view::cocos
