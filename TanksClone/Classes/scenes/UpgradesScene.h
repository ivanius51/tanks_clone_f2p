#pragma once

#include <string>
#include <vector>

#include "cocos2d.h"

#include "model/MathTypes.h"
#include "model/UpgradeTree.h"
#include "scenes/TypedScene.h"

namespace tanks { struct UpgradeDef; }

namespace tanks::view::cocos
{

namespace widgets { class Button; }

/**
 * The shop as a blueprint of the player's tank, seen from the top and from the side. Tank upgrades are round nodes
 * linked to the part they improve; the weapons are a row of tabs above the turret, and the picked weapon's upgrades
 * show in a box. Arrows or a tap move the focus, the side panel tells what the focused upgrade does and buys it.
 */
class UpgradesScene : public TypedScene<SceneKind::Upgrades>
{
public:
	static cocos2d::Scene* CreateScene();

	bool init() override;

	CREATE_FUNC(UpgradesScene);

private:
	struct NodeView
	{
		cocos2d::Node* Root = nullptr;
		cocos2d::DrawNode* Frame = nullptr;
		cocos2d::Node* Badge = nullptr;
		cocos2d::Label* Price = nullptr;
	};

	void BuildBlueprint();
	void BuildTree();
	void BuildTabs();
	// The weapon buttons fan out from the turret, or fold back onto it leaving only the selected one.
	void SetWeaponsOpen(bool bInOpen);
	cocos2d::Vec2 GetTabTarget(int InTab) const;
	void BuildPanel();
	void AddInput();
	NodeView CreateNodeView(const std::string& InIcon, const std::string& InNameKey, const cocos2d::Vec2& InPosition);
	void Refresh();
	void RefreshNode(int InIndex);
	void RefreshTab(int InTab);
	void RefreshPanel();
	void SetFocus(int InIndex);
	void Move(const model::Vec2f& InDirection);
	void Confirm();
	bool IsShown(int InIndex) const;
	bool IsTab(int InIndex) const;
	bool IsWeaponBranch(const std::string& InBranch) const;
	bool IsWeaponUnlocked(const std::string& InBranch) const;
	int GetFocusCount() const;
	cocos2d::Vec2 GetFocusPosition(int InIndex) const;
	int FindNodeAt(const cocos2d::Vec2& InPoint) const;
	std::string GetRequirementText(const UpgradeDef& InUpgrade) const;

	model::UpgradeTree Tree;
	std::vector<const UpgradeDef*> Upgrades; // one per tree node
	std::vector<NodeView> Views;
	std::vector<NodeView> Tabs; // focus indices after the tree nodes
	std::string SelectedWeapon;
	std::vector<cocos2d::Vec2> TabHomes;
	bool bWeaponsOpen = false;
	cocos2d::DrawNode* Links = nullptr;
	cocos2d::DrawNode* FocusRing = nullptr;
	cocos2d::Label* BoxTitle = nullptr;
	cocos2d::Label* Coins = nullptr;
	cocos2d::Label* Name = nullptr;
	cocos2d::Label* Info = nullptr;
	cocos2d::Label* Level = nullptr;
	cocos2d::Label* Need = nullptr;
	widgets::Button* Buy = nullptr;
	int Focused = 0;
};

} // namespace tanks::view::cocos
