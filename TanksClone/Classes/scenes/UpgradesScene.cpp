#include "scenes/UpgradesScene.h"

#include <string>

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

	float Y = DesignHeight - Layout().FirstRowTop;
	for ( const auto& Upgrade : CurrentUpgrades.GetUpgrades() )
	{
		const std::string CurrentId = Upgrade.Id;
		const int CurrentLevel = CurrentUpgrades.GetLevel(CurrentId);
		const int Cost = CurrentUpgrades.GetNextCost(CurrentId);

		auto CurrentName = widgets::CreateLabel(Text(Upgrade.NameKey), Layout().NameFontSize);
		CurrentName->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
		CurrentName->setPosition(Layout().NameX, Y + Layout().NameOffsetY);
		addChild(CurrentName);

		auto Info = widgets::CreateLabel(Text(Upgrade.NameKey + "_INFO"), Layout().InfoFontSize, widgets::colors::TextDim());
		Info->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
		Info->setPosition(Layout().NameX, Y + Layout().InfoOffsetY);
		addChild(Info);

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
		addChild(Pips);

		const std::string Caption = Cost < 0 ? Text("UPGRADE_MAX") : Text("UPGRADE_BUY") + "  " + std::to_string(Cost);
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
		addChild(CurrentBuy, 1);
		List->AddButton(CurrentBuy);

		Y -= Layout().RowStep;
	}

	auto Back = widgets::Button::create(Text("MENU_BACK"), widgets::ScaleUi(cocos2d::Size(Layout().BuyWidth, Layout().BuyHeight)), []() { SwitchScene(MenuScene::CreateScene()); });
	Back->setPosition(DesignWidth / 2, Layout().BackBottom);
	addChild(Back, 1);
	List->AddButton(Back);

	// Keep the keyboard selection where it was, on the nearest button that can be used.
	Selected = InSelectedButton;
	List->Select(InSelectedButton);
}

} // namespace tanks::view::cocos
