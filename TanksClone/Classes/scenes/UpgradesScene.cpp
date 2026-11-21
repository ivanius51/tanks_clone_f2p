#include "scenes/UpgradesScene.h"

#include <algorithm>
#include <string>
#include <vector>

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "scenes/MenuScene.h"
#include "scenes/SceneUtils.h"
#include "services/AudioManager.h"
#include "services/ProfileManager.h"
#include "services/StringsManager.h"
#include "services/UpgradeManager.h"
#include "services/UiStyle.h"
#include "ui/Widgets.h"

namespace tanks::view::cocos
{

namespace
{
	const viewmodel::UpgradesLayout& Layout()
	{
		return GameContext::Get().GetManager<UiStyle>().GetMenu().Upgrades;
	}


	std::string Text(const std::string& InId)
	{
		return GameContext::Get().GetManager<StringsManager>().GetString(InId);
	}
}

cocos2d::Scene* UpgradesScene::CreateScene()
{
	return UpgradesScene::create();
}

bool UpgradesScene::init()
{
	if ( !Scene::init() )
	{
		return false;
	}
	Build(0);
	auto Wheel = cocos2d::EventListenerMouse::create();
	Wheel->onMouseScroll = [this](cocos2d::EventMouse* const InEvent) { Scroll(InEvent->getScrollY() > 0.0f ? 1 : -1); };
	getEventDispatcher()->addEventListenerWithSceneGraphPriority(Wheel, this);
	scheduleUpdate();
	return true;
}

void UpgradesScene::Build(const int InSelectedButton)
{
	removeAllChildren();
	addChild(widgets::CreateScreenBackground());
	widgets::AddBackKeyHandler(this, []() { SwitchScene(MenuScene::CreateScene()); });

	auto Title = widgets::CreateLabel(Text("UPGRADES_TITLE"), Layout().TitleFontSize, widgets::colors::Accent(), true);
	Title->setPosition(DesignWidth / 2, DesignHeight - Layout().TitleTop);
	addChild(Title);

	auto CurrentCoins = widgets::CreateLabel(Text("UPGRADES_COINS") + ": " + std::to_string(GameContext::Get().GetManager<ProfileManager>().Get().Coins),
		Layout().NameFontSize, widgets::colors::Accent());
	CurrentCoins->setPosition(DesignWidth / 2, DesignHeight - Layout().CoinsTop);
	addChild(CurrentCoins);

	auto& CurrentUpgrades = GameContext::Get().GetManager<UpgradeManager>();
	auto List = widgets::ButtonList::create();
	addChild(List);

	Rows.clear();
	for ( const auto& Upgrade : CurrentUpgrades.GetUpgrades() )
	{
		auto Row = cocos2d::Node::create();
		addChild(Row, 1);
		Rows.push_back(Row);
		const float Y = 0.0f;
		const std::string CurrentId = Upgrade.Id;
		const int CurrentLevel = CurrentUpgrades.GetLevel(CurrentId);
		const int Cost = CurrentUpgrades.GetNextCost(CurrentId);

		auto CurrentName = widgets::CreateLabel(Text(Upgrade.NameKey), Layout().NameFontSize);
		CurrentName->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
		CurrentName->setPosition(Layout().NameX, Y + Layout().NameOffsetY);
		Row->addChild(CurrentName);

		const bool bIsOpen = CurrentUpgrades.IsOpen(CurrentId);
		auto Info = widgets::CreateLabel(bIsOpen ? Text(Upgrade.NameKey + "_INFO") : GetRequirementText(Upgrade), Layout().InfoFontSize, widgets::colors::TextDim());
		Info->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
		Info->setPosition(Layout().NameX, Y + Layout().InfoOffsetY);
		Row->addChild(Info);

		// One square per level: filled for the levels that are bought.
		auto Pips = cocos2d::DrawNode::create();
		for ( int Index = 0; Index < Upgrade.MaxLevel; ++Index )
		{
			const cocos2d::Vec2 Low(Layout().PipsX + Index * Layout().PipStep, Y - Layout().PipSize / 2);
			const cocos2d::Vec2 High = Low + cocos2d::Vec2(Layout().PipSize, Layout().PipSize);
			Pips->drawSolidRect(Low, High, cocos2d::Color4F(ToCocos(Layout().PipEdge)));
			Pips->drawSolidRect(Low + cocos2d::Vec2(Layout().PipBorder, Layout().PipBorder), High - cocos2d::Vec2(Layout().PipBorder, Layout().PipBorder),
				Index < CurrentLevel ? cocos2d::Color4F(ToCocos(Layout().PipFull)) : cocos2d::Color4F(ToCocos(Layout().PipEmpty)));
		}
		Row->addChild(Pips);

		const std::string Caption = Cost < 0 ? Text("UPGRADE_MAX") : (bIsOpen ? Text("UPGRADE_BUY") + "  " + std::to_string(Cost) : Text("UPGRADE_LOCKED"));
		auto CurrentBuy = widgets::Button::create(Caption, widgets::ScaleUi(cocos2d::Size(Layout().BuyWidth, Layout().BuyHeight)), [this, CurrentId, List]()
		{
			Selected = List->GetSelected();
			if ( GameContext::Get().GetManager<UpgradeManager>().Buy(CurrentId) )
			{
				GameContext::Get().GetManager<AudioManager>().Play(ids::sounds::BonusPickup);
			}
			// Rebuild on the next frame: this button is still running its callback.
			scheduleOnce([this](float) { Build(Selected); }, 0.0f, "rebuild");
		});
		CurrentBuy->setPosition(Layout().BuyX, Y);
		CurrentBuy->SetEnabled(CurrentUpgrades.CanBuy(CurrentId));
		Row->addChild(CurrentBuy, 1);
		List->AddButton(CurrentBuy);
	}

	auto Back = widgets::Button::create(Text("MENU_BACK"), widgets::ScaleUi(cocos2d::Size(Layout().BuyWidth, Layout().BuyHeight)), []() { SwitchScene(MenuScene::CreateScene()); });
	Back->setPosition(DesignWidth / 2, Layout().BackBottom);
	addChild(Back, 1);
	List->AddButton(Back);

	// Keep the keyboard selection where it was, on the nearest button that can be used.
	Selected = InSelectedButton;
	List->Select(InSelectedButton);
	Buttons = List;
	CreateArrows();
	ScrollToSelected();
	PlaceRows();
}

void UpgradesScene::update(const float InDelta)
{
	TypedScene::update(InDelta);
	if ( Buttons && Buttons->GetSelected() != Selected )
	{
		Selected = Buttons->GetSelected();
		ScrollToSelected();
		PlaceRows();
	}
}

std::string UpgradesScene::GetRequirementText(const UpgradeDef& InUpgrade) const
{
	const UpgradeDef* const Requirement = GameContext::Get().GetManager<UpgradeManager>().GetRequirement(InUpgrade.Id);
	if ( !Requirement )
	{
		return std::string();
	}
	const std::string Level = InUpgrade.RequiresLevel > 1 ? " " + Text("UPGRADE_LEVEL") + " " + std::to_string(InUpgrade.RequiresLevel) : std::string();
	return Text("UPGRADE_NEEDS") + ": " + Text(Requirement->NameKey) + Level;
}

int UpgradesScene::GetVisibleRows() const
{
	const float Top = DesignHeight - Layout().FirstRowTop + Layout().RowStep / 2;
	const float Bottom = Layout().BackBottom + Layout().BuyHeight / 2;
	return std::max(1, static_cast<int>((Top - Bottom) / Layout().RowStep));
}

void UpgradesScene::ScrollToSelected()
{
	const int RowCount = static_cast<int>(Rows.size());
	if ( Selected < 0 || Selected >= RowCount )
	{
		return;
	}
	if ( Selected < FirstRow )
	{
		FirstRow = Selected;
	}
	else if ( Selected >= FirstRow + GetVisibleRows() )
	{
		FirstRow = Selected - GetVisibleRows() + 1;
	}
}

void UpgradesScene::Scroll(const int InRows)
{
	FirstRow += InRows;
	PlaceRows();
}

void UpgradesScene::PlaceRows()
{
	const int RowCount = static_cast<int>(Rows.size());
	FirstRow = std::clamp(FirstRow, 0, std::max(0, RowCount - GetVisibleRows()));
	for ( int Index = 0; Index < RowCount; ++Index )
	{
		const int Slot = Index - FirstRow;
		Rows[Index]->setVisible(Slot >= 0 && Slot < GetVisibleRows());
		Rows[Index]->setPositionY(DesignHeight - Layout().FirstRowTop - Slot * Layout().RowStep);
	}
	ArrowUp->setVisible(FirstRow > 0);
	ArrowDown->setVisible(FirstRow + GetVisibleRows() < RowCount);
}

void UpgradesScene::CreateArrows()
{
	const float Size = Layout().ArrowSize;
	const cocos2d::Color4F Color(ToCocos(Layout().ArrowColor));
	const float TopY = DesignHeight - Layout().FirstRowTop + Layout().RowStep / 2 - Layout().ArrowGap - Size;
	ArrowUp = cocos2d::DrawNode::create();
	ArrowUp->drawSolidPoly(std::vector<cocos2d::Vec2>{ { -Size, 0.0f }, { Size, 0.0f }, { 0.0f, Size } }.data(), 3, Color);
	ArrowUp->setPosition(DesignWidth / 2, TopY);
	addChild(ArrowUp, 1);

	const int Shown = std::min(GetVisibleRows(), static_cast<int>(Rows.size()));
	const float BottomY = DesignHeight - Layout().FirstRowTop - (Shown - 0.5f) * Layout().RowStep + Layout().ArrowGap + Size;
	ArrowDown = cocos2d::DrawNode::create();
	ArrowDown->drawSolidPoly(std::vector<cocos2d::Vec2>{ { -Size, 0.0f }, { Size, 0.0f }, { 0.0f, -Size } }.data(), 3, Color);
	ArrowDown->setPosition(DesignWidth / 2, BottomY);
	addChild(ArrowDown, 1);
}

} // namespace tanks::view::cocos
