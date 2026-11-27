#pragma once

#include "cocos2d.h"

#include "game/objects/Tank.h"
#include "scenes/TypedScene.h"
#include "templates/TContainers.h"
#include "ui/Widgets.h"

namespace game
{
class TankCondition;
}

namespace game::view::cocos
{
class LayoutWidget;

class MenuScene : public TypedScene<SceneKind::Menu>
{
public:
	static cocos2d::Scene* CreateScene();

	bool init() override;
	void onExit() override;

	CREATE_FUNC(MenuScene);

private:
	void StartNewGame();
	void ContinueGame();
	void AskNewGame();
	void ShowDebugMenu();
	void ShowGalleries();
	void ShowStatus();
	void ShowGarageTank(LayoutWidget& InGarage, const TankCondition& InCondition);
	void ShowCredits();
	void ShowCustomMenu(const TArray<widgets::Overlay::ButtonSpec>& InCampaignButtons);

	LayoutWidget* Group = nullptr;
	cocos2d::Node* GarageStage = nullptr;
	FTankHandle GarageTank;
};
} // namespace game::view::cocos
