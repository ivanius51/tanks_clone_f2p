#pragma once

#include <vector>

#include "cocos2d.h"

#include "scenes/TypedScene.h"

namespace tanks::view::cocos
{

namespace widgets { class ButtonList; }

/** The shop where coins are spent on upgrades for the player's tank; rows scroll when they don't fit. */
class UpgradesScene : public TypedScene<SceneKind::Upgrades>
{
public:
	static cocos2d::Scene* CreateScene();

	bool init() override;
	void update(float InDelta) override;

	CREATE_FUNC(UpgradesScene);

private:
	void Build(int InSelectedButton);
	int GetVisibleRows() const;
	void ScrollToSelected();
	void Scroll(int InRows);
	void PlaceRows();
	void CreateArrows();

	std::vector<cocos2d::Node*> Rows;
	cocos2d::DrawNode* ArrowUp = nullptr;
	cocos2d::DrawNode* ArrowDown = nullptr;
	widgets::ButtonList* Buttons = nullptr;
	int Selected = 0;
	int FirstRow = 0;
};

} // namespace tanks::view::cocos
