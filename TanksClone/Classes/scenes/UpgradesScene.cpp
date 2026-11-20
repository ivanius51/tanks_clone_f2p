#include "scenes/UpgradesScene.h"

#include "services/AudioManager.h"
#include "core/GameConstants.h"
#include "scenes/MenuScene.h"
#include "services/ProfileManager.h"
#include "scenes/SceneUtils.h"
#include "services/StringsManager.h"
#include "services/UpgradeManager.h"
#include "ui/Widgets.h"

namespace tanks::view::cocos
{

namespace
{
	constexpr float TitleFontSize = 56.0f;
	constexpr float NameFontSize = 28.0f;
	constexpr float InfoFontSize = 18.0f;
	constexpr float FirstRowY = DesignHeight - 235.0f;
	constexpr float RowStep = 100.0f;
	constexpr float NameX = 110.0f;
	constexpr float PipsX = 520.0f;
	constexpr float PipSize = 18.0f;
	constexpr float PipStep = 26.0f;
	constexpr float BuyX = 850.0f;
	constexpr float BuyWidth = 220.0f;
	constexpr float BuyHeight = 56.0f;

	std::string Text(const std::string& InId)
	{
		return StringsManager::GetInstance().GetString(InId);
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

	auto Title = widgets::CreateLabel(Text("UPGRADES_TITLE"), TitleFontSize, widgets::colors::Accent, true);
	Title->setPosition(DesignWidth / 2, DesignHeight - 80);
	addChild(Title);

	auto CurrentCoins = widgets::CreateLabel(Text("UPGRADES_COINS") + ": " + std::to_string(ProfileManager::GetInstance().Get().Coins),
		NameFontSize, widgets::colors::Accent);
	CurrentCoins->setPosition(DesignWidth / 2, DesignHeight - 150);
	addChild(CurrentCoins);

	auto& CurrentUpgrades = UpgradeManager::GetInstance();
	auto List = widgets::ButtonList::create();
	addChild(List);

	float Y = FirstRowY;
	for ( const auto& Upgrade : CurrentUpgrades.GetUpgrades() )
	{
		const std::string CurrentId = Upgrade.Id;
		const int CurrentLevel = CurrentUpgrades.GetLevel(CurrentId);
		const int Cost = CurrentUpgrades.GetNextCost(CurrentId);

		auto CurrentName = widgets::CreateLabel(Text(Upgrade.NameKey), NameFontSize);
		CurrentName->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
		CurrentName->setPosition(NameX, Y + 12);
		addChild(CurrentName);

		auto Info = widgets::CreateLabel(Text(Upgrade.NameKey + "_INFO"), InfoFontSize, widgets::colors::TextDim);
		Info->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
		Info->setPosition(NameX, Y - 18);
		addChild(Info);

		// One square per level: filled for the levels that are bought.
		auto Pips = cocos2d::DrawNode::create();
		for ( int Index = 0; Index < Upgrade.MaxLevel; ++Index )
		{
			const cocos2d::Vec2 Low(PipsX + Index * PipStep, Y - PipSize / 2);
			const cocos2d::Vec2 High = Low + cocos2d::Vec2(PipSize, PipSize);
			Pips->drawSolidRect(Low, High, cocos2d::Color4F(0.35f, 0.4f, 0.5f, 1.0f));
			Pips->drawSolidRect(Low + cocos2d::Vec2(2, 2), High - cocos2d::Vec2(2, 2),
				Index < CurrentLevel ? cocos2d::Color4F(1.0f, 0.8f, 0.25f, 1.0f) : cocos2d::Color4F(0.1f, 0.11f, 0.15f, 1.0f));
		}
		addChild(Pips);

		const std::string Caption = Cost < 0 ? Text("UPGRADE_MAX") : Text("UPGRADE_BUY") + "  " + std::to_string(Cost);
		auto CurrentBuy = widgets::Button::create(Caption, cocos2d::Size(BuyWidth, BuyHeight), [this, CurrentId, List]()
		{
			Selected = List->GetSelected();
			if ( UpgradeManager::GetInstance().Buy(CurrentId) )
			{
				AudioManager::GetInstance().Play(Sound::BonusPickup);
			}
			// Rebuild on the next frame: this button is still running its callback.
			scheduleOnce([this](float) { Build(Selected); }, 0.0f, "rebuild");
		});
		CurrentBuy->setPosition(BuyX, Y);
		CurrentBuy->SetEnabled(CurrentUpgrades.CanBuy(CurrentId));
		addChild(CurrentBuy, 1);
		List->AddButton(CurrentBuy);

		Y -= RowStep;
	}

	auto Back = widgets::Button::create(Text("MENU_BACK"), cocos2d::Size(BuyWidth, BuyHeight), []() { SwitchScene(MenuScene::CreateScene()); });
	Back->setPosition(DesignWidth / 2, 56.0f);
	addChild(Back, 1);
	List->AddButton(Back);

	// Keep the keyboard selection where it was, on the nearest button that can be used.
	Selected = InSelectedButton;
	List->Select(InSelectedButton);
}

} // namespace tanks::view::cocos