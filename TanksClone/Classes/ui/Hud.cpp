#include "ui/Hud.h"

#include <algorithm>
#include <cmath>

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "game/objects/Tank.h"
#include "game/ObjectsFactory.h"
#include "services/InputManager.h"
#include "services/StringsManager.h"
#include "services/UiStyle.h"
#include "ui/CocosConvert.h"
#include "ui/ScreenFit.h"
#include "ui/WidgetFactory.h"
#include "ui/Widgets.h"

namespace game::view::cocos
{

namespace
{
	const viewmodel::HudLayout& Layout()
	{
		return GameContext::Get<UiStyle>().GetHud();
	}

	cocos2d::Color4F Paint(const model::Color& InColor)
	{
		return cocos2d::Color4F(ToCocos(InColor));
	}

	// Top bar

	// Player panel



	std::string Text(const std::string& InId)
	{
		return GameContext::Get<StringsManager>().GetString(InId);
	}

	void DrawSlotFrame(cocos2d::DrawNode* const InNode, const float InSize, const cocos2d::Color4F& InBorder)
	{
		const cocos2d::Vec2 Low(-InSize / 2, -InSize / 2);
		const cocos2d::Vec2 High(InSize / 2, InSize / 2);
		InNode->clear();
		InNode->drawSolidRect(Low, High, InBorder);
		InNode->drawSolidRect(Low + cocos2d::Vec2(Layout().FrameBorder, Layout().FrameBorder), High - cocos2d::Vec2(Layout().FrameBorder, Layout().FrameBorder), Paint(Layout().Colors.SlotFill));
	}
}

Hud* Hud::create(const model::GameSession* const InSession)
{
	Hud* NewHud = new (std::nothrow) Hud();
	if ( NewHud && NewHud->init(InSession) )
	{
		NewHud->autorelease();
		return NewHud;
	}
	CC_SAFE_DELETE(NewHud);
	return nullptr;
}

bool Hud::init(const model::GameSession* const InSession)
{
	if ( !Node::init() )
	{
		return false;
	}
	Session = InSession;

	BuildTopBar();
	BuildPlayerPanel();
	OnScreenLayoutChanged(this, [this](const model::ScreenLayout& InLayout) { Relayout(InLayout); });

	scheduleUpdate();
	update(0.0f);
	return true;
}

cocos2d::Label* Hud::AddLabel(Node* const InParent, const cocos2d::Vec2& InPosition, const float InFontSize, const cocos2d::Vec2& InAnchor)
{
	auto CurrentLabel = widgets::CreateLabel("", InFontSize);
	CurrentLabel->setAnchorPoint(InAnchor);
	CurrentLabel->setPosition(InPosition);
	InParent->addChild(CurrentLabel, 1);
	return CurrentLabel;
}

cocos2d::Sprite* Hud::CreateIcon(const std::string& InFrame, const cocos2d::Color3B& InColor, const float InSize)
{
	auto CurrentSprite = widgets::CreateFrameSprite(InFrame);
	if ( !CurrentSprite )
	{
		return cocos2d::Sprite::create();
	}
	CurrentSprite->setColor(InColor);
	CurrentSprite->setScale(InSize / std::max(1.0f, std::max(CurrentSprite->getContentSize().width, CurrentSprite->getContentSize().height)));
	return CurrentSprite;
}

cocos2d::Node* Hud::CreateAbilityIcon(const Ability& InAbility, const float InSize)
{
	return CreateIcon(InAbility.GetIconFrame(), InAbility.GetIconColor(), InSize);
}

// ---------------------------------------------------------------- top bar

void Hud::BuildTopBar()
{
	Top = Node::create();
	addChild(Top);
	TopBackground = cocos2d::DrawNode::create();
	Top->addChild(TopBackground);

	const float Y = (TopHudHeight - Layout().BandBorder) / 2;
	LevelLabel = AddLabel(Top, cocos2d::Vec2(Layout().TopPadding, Y), Layout().TopFontSize);
	LevelLabel->setColor(widgets::colors::Accent());
	WaveLabel = AddLabel(Top, cocos2d::Vec2(Layout().WaveX, Y), Layout().TopFontSize);
	EnemiesLabel = AddLabel(Top, cocos2d::Vec2(Layout().EnemiesX, Y), Layout().TopSmallFontSize);
	EnemiesLabel->setColor(widgets::colors::TextDim());
	EnemiesBar = WidgetFactory::Create(ids::widgets::HudEnemiesBar);
	EnemiesBar->setPosition(Layout().EnemiesBarX + EnemiesBar->getContentSize().width / 2, Y);
	Top->addChild(EnemiesBar);
	ScoreLabel = AddLabel(Top, cocos2d::Vec2(DesignWidth - Layout().TopPadding, Y), Layout().TopFontSize, cocos2d::Vec2(1.0f, 0.5f));
	ScoreLabel->setColor(widgets::colors::Accent());

	if ( Session )
	{
		LevelLabel->setString(Session->LevelName);
	}
}

void Hud::UpdateTopBar()
{
	if ( !Session )
	{
		return;
	}

	const std::string CurrentWave = Session->WaveCount > 1
		? Text("WAVE_WORD") + " " + std::to_string(Session->WaveNumber) + "/" + std::to_string(Session->WaveCount) : "";
	if ( CurrentWave != ShownWave )
	{
		ShownWave = CurrentWave;
		WaveLabel->setString(CurrentWave);
	}

	const std::string CurrentEnemies = Text("HUD_ENEMIES") + " " + std::to_string(Session->Defeated) + "/" + std::to_string(Session->TotalEnemies);
	if ( CurrentEnemies != ShownEnemies )
	{
		ShownEnemies = CurrentEnemies;
		EnemiesLabel->setString(CurrentEnemies);
	}
	const int Percent = Session->TotalEnemies > 0 ? 100 * Session->Defeated / Session->TotalEnemies : 0;
	if ( Percent != ShownEnemiesPercent )
	{
		ShownEnemiesPercent = Percent;
		EnemiesBar->SetPartFraction("bar", Percent / 100.0f);
	}

	const std::string CurrentScore = Text("HUD_SCORE") + " " + std::to_string(Session->Score)
		+ "     " + Text("HUD_COINS") + " " + std::to_string(Session->Coins);
	if ( CurrentScore != ShownScore )
	{
		ShownScore = CurrentScore;
		ScoreLabel->setString(CurrentScore);
	}
}

// ----------------------------------------------------------- player panel

void Hud::BuildPlayerPanel()
{
	Panel = Node::create();
	addChild(Panel);
	PanelBackground = cocos2d::DrawNode::create();
	Panel->addChild(PanelBackground);

	// Portrait
	const cocos2d::Vec2 PortraitCenter(Layout().PortraitX + Layout().PortraitSize / 2, (BottomHudHeight - Layout().BandBorder) / 2);
	auto CurrentFrame = cocos2d::DrawNode::create();
	CurrentFrame->setPosition(PortraitCenter);
	DrawSlotFrame(CurrentFrame, Layout().PortraitSize, Paint(Layout().Colors.PanelBorder));
	Panel->addChild(CurrentFrame);
	auto Portrait = CreateIcon("player_tank", cocos2d::Color3B::WHITE, Layout().PortraitSize - Layout().PortraitInset);
	Portrait->setPosition(PortraitCenter);
	Panel->addChild(Portrait, 1);

	// Bars and stats
	const auto AddBar = [this](const std::string_view InWidget, const float InBottom) -> LayoutWidget*
	{
		LayoutWidget* const NewBar = WidgetFactory::Create(InWidget);
		NewBar->setPosition(Layout().BarsX + NewBar->getContentSize().width / 2, InBottom + NewBar->getContentSize().height / 2);
		Panel->addChild(NewBar, 1);
		return NewBar;
	};
	HealthBar = AddBar(ids::widgets::HudHealthBar, Layout().HealthY);
	AmmoBar = AddBar(ids::widgets::HudAmmoBar, Layout().AmmoY);
	ReloadBar = AddBar(ids::widgets::HudReloadBar, Layout().ReloadY);
	StatsLabel = AddLabel(Panel, cocos2d::Vec2(Layout().BarsX, Layout().StatsY), Layout().StatsFontSize);
	StatsLabel->setColor(widgets::colors::TextDim());

	SlotsRoot = Node::create();
	Panel->addChild(SlotsRoot);
}

void Hud::DrawBand(cocos2d::DrawNode* const InNode, const float InWidth, const float InHeight)
{
	InNode->clear();
	InNode->drawSolidRect(cocos2d::Vec2(0, 0), cocos2d::Vec2(InWidth, InHeight), Paint(Layout().Colors.PanelFill));
	InNode->drawSolidRect(cocos2d::Vec2(0, InHeight - Layout().BandBorder), cocos2d::Vec2(InWidth, InHeight), Paint(Layout().Colors.PanelBorder));
}

void Hud::Relayout(const model::ScreenLayout& InLayout)
{
	const float Width = InLayout.GetVisibleSize().X;
	Top->setPosition(ToCocos(InLayout.Place(Layout().TopAnchor, model::Vec2f(0.0f, -TopHudHeight))));
	DrawBand(TopBackground, Width, TopHudHeight);
	ScoreLabel->setPositionX(Width - Layout().TopPadding);

	Panel->setPosition(ToCocos(InLayout.GetAnchor(Layout().PanelAnchor)));
	DrawBand(PanelBackground, Width, BottomHudHeight);
	SlotsRoot->setPositionX(InLayout.GetDesignOffset().X);
}

void Hud::BuildSlots(Tank& InPlayer)
{
	bSlotsBuilt = true;

	const float Y = (BottomHudHeight - Layout().BandBorder) / 2 - Layout().SlotsOffsetY;
	const InputManager& Input = GameContext::Get<InputManager>();
	const std::string FireKey = Input.GetButtonLabel(model::InputContext::Gameplay, model::InputAction::Fire);
	const std::string AbilityKey = Input.GetButtonLabel(model::InputContext::Gameplay, model::InputAction::Ability1);
	const std::string SwitchKeyText = Input.GetButtonLabel(model::InputContext::Gameplay, model::InputAction::SwitchWeapon);
	for ( size_t Index = 0; Index < std::min(InPlayer.GetAbilityCount(), static_cast<size_t>(std::max(0, Layout().MaxSlots))); ++Index )
	{
		const Ability* CurrentAbility = InPlayer.GetAbility(Index);

		auto CurrentRoot = Node::create();
		CurrentRoot->setPosition(Layout().SlotsX + Index * Layout().SlotStep, Y);
		SlotsRoot->addChild(CurrentRoot);

		auto CurrentFrame = cocos2d::DrawNode::create();
		DrawSlotFrame(CurrentFrame, Layout().SlotSize, Paint(Layout().Colors.PanelBorder));
		CurrentRoot->addChild(CurrentFrame);

		Slot CurrentSlot;
		CurrentSlot.Root = CurrentRoot;
		CurrentSlot.Icon = CreateAbilityIcon(*CurrentAbility, Layout().SlotIconSize);
		CurrentSlot.ShownIcon = CurrentAbility->GetIconFrame();
		CurrentSlot.Icon->setPosition(0, Layout().SlotIconOffsetY);
		CurrentRoot->addChild(CurrentSlot.Icon, 1);

		if ( Index == 0 && InPlayer.GetWeaponCount() > 1 )
		{
			if ( auto SwitchButton = CreateSwitchButton(SwitchKeyText) )
			{
				SwitchButton->setPosition(0, Layout().SlotSize / 2 + SwitchButton->getContentSize().height / 2);
				CurrentRoot->addChild(SwitchButton, 5);
			}
		}

		CurrentSlot.Cooldown = cocos2d::DrawNode::create();
		CurrentRoot->addChild(CurrentSlot.Cooldown, 2);

		// Slot 0 is the primary ability (fire), the rest are used with the ability action.
		const std::string& HotkeyText = Index == 0 ? FireKey : AbilityKey;
		if ( auto Hotkey = WidgetFactory::CreateKeycap(HotkeyText) )
		{
			Hotkey->setPosition(0, -Layout().SlotSize / 2 + Layout().KeyInsetY + Hotkey->getContentSize().height / 2);
			CurrentRoot->addChild(Hotkey, 4);
		}

		CurrentSlot.Timer = widgets::CreateLabel("", Layout().TimerFontSize);
		widgets::AddOutline(CurrentSlot.Timer);
		CurrentRoot->addChild(CurrentSlot.Timer, 3);

		Slots.push_back(CurrentSlot);
	}
}

widgets::Button* Hud::CreateSwitchButton(const std::string& InKeyText)
{
	LayoutWidget* const Bar = WidgetFactory::Create(ids::widgets::SwitchBar);
	if ( !Bar )
	{
		return nullptr;
	}
	Bar->FindWidget("key")->SetText("text", InKeyText);
	auto Result = widgets::Button::create("", Bar->getContentSize(), []()
	{
		auto Player = GameContext::Get<ObjectsFactory>().FindObject<Tank>(PlayerName);
		if ( Player )
		{
			Player->OnCommandReceived(GameCommand(CommandType::SwitchWeapon));
		}
	});
	Result->addChild(Bar, 2);
	return Result;
}

void Hud::UpdatePanel(Tank* const InPlayer)
{
	// Health
	const int CurrentHealth = InPlayer ? InPlayer->GetHealth() : 0;
	const int CurrentMaxHealth = InPlayer ? InPlayer->GetMaxHealth() : 1;
	const std::string HealthText = Text("HUD_HEALTH") + "  " + std::to_string(CurrentHealth) + " / " + std::to_string(CurrentMaxHealth);
	const int HealthPercent = 100 * CurrentHealth / std::max(1, CurrentMaxHealth);
	if ( HealthText != ShownHealth )
	{
		ShownHealth = HealthText;
		HealthBar->SetText("text", HealthText);
	}
	if ( HealthPercent != ShownHealthPercent )
	{
		ShownHealthPercent = HealthPercent;
		HealthBar->SetPartFraction("bar", HealthPercent / 100.0f);
		HealthBar->SetPartFillColor("bar", HealthPercent <= Layout().HealthLowPercent ? Layout().Colors.HealthLowColor : Layout().Colors.HealthColor);
	}

	// Ammo
	const bool bFreeWeapon = InPlayer && !InPlayer->GetActiveWeapon().bUsesAmmo;
	const bool bUnlimited = InPlayer && (InPlayer->HasUnlimitedAmmo() || bFreeWeapon);
	const int CurrentAmmo = InPlayer ? InPlayer->GetAmmo() : 0;
	const int MaxAmmo = InPlayer ? InPlayer->GetMaxAmmo() : 0;
	const std::string AmmoTitle = bFreeWeapon ? Text(InPlayer->GetActiveWeapon().NameKey) : Text("HUD_AMMO");
	const std::string AmmoText = AmmoTitle + "  " + (bUnlimited ? std::string("INF") : std::to_string(CurrentAmmo) + " / " + std::to_string(MaxAmmo));
	if ( AmmoText != ShownAmmo )
	{
		ShownAmmo = AmmoText;
		AmmoBar->SetText("text", AmmoText);
	}
	const int AmmoPercent = bUnlimited ? 100 : (MaxAmmo > 0 ? 100 * CurrentAmmo / MaxAmmo : 0);
	if ( AmmoPercent != ShownAmmoPercent )
	{
		ShownAmmoPercent = AmmoPercent;
		AmmoBar->SetPartFraction("bar", AmmoPercent / 100.0f);
		AmmoBar->SetPartFillColor("bar", CurrentAmmo == 0 && !bUnlimited ? Layout().Colors.AmmoEmptyColor : Layout().Colors.AmmoColor);
	}

	// Reload
	const int Reload = InPlayer ? static_cast<int>(InPlayer->GetReloadProgress() * 100) : 0;
	if ( Reload != ShownReloadPercent )
	{
		ShownReloadPercent = Reload;
		ReloadBar->SetPartFraction("bar", Reload / 100.0f);
	}

	// Stats
	std::string CurrentStats;
	if ( InPlayer )
	{
		CurrentStats = Text("HUD_ARMOR") + " " + std::to_string(InPlayer->GetArmor())
			+ "     " + Text("HUD_DAMAGE") + " " + std::to_string(InPlayer->GetDamage())
			+ "     " + Text("HUD_SPEED") + " " + std::to_string(InPlayer->GetStats().GetInt(StatType::Speed));
	}
	if ( CurrentStats != ShownStats )
	{
		ShownStats = CurrentStats;
		StatsLabel->setString(CurrentStats);
	}
}

void Hud::UpdateSlots(Tank* const InPlayer)
{
	const model::InputDevice Device = GameContext::Get<InputManager>().GetLastDevice();
	if ( bSlotsBuilt && Device != SlotsDevice )
	{
		SlotsRoot->removeAllChildren();
		Slots.clear();
		bSlotsBuilt = false;
	}
	if ( InPlayer && !bSlotsBuilt )
	{
		SlotsDevice = Device;
		BuildSlots(*InPlayer);
	}

	for ( size_t Index = 0; Index < Slots.size(); ++Index )
	{
		Slot& CurrentSlot = Slots[Index];
		const Ability* CurrentAbility = InPlayer ? InPlayer->GetAbility(Index) : nullptr;
		if ( !CurrentAbility )
		{
			continue;
		}

		if ( CurrentAbility->GetIconFrame() != CurrentSlot.ShownIcon )
		{
			CurrentSlot.ShownIcon = CurrentAbility->GetIconFrame();
			CurrentSlot.Icon->removeFromParent();
			CurrentSlot.Icon = CreateAbilityIcon(*CurrentAbility, Layout().SlotIconSize);
			CurrentSlot.Icon->setPosition(0, Layout().SlotIconOffsetY);
			CurrentSlot.Root->addChild(CurrentSlot.Icon, 1);
		}

		const float CurrentRemaining = CurrentAbility->GetCooldownRemaining();
		const int Ready = static_cast<int>(CurrentAbility->GetReadiness() * 100);
		if ( Ready != CurrentSlot.ShownCooldown )
		{
			CurrentSlot.ShownCooldown = Ready;
			// The dark shade covers the part of the icon that is still recharging.
			CurrentSlot.Cooldown->clear();
			if ( Ready < 100 )
			{
				const float CurrentHeight = Layout().SlotSize * (1.0f - Ready / 100.0f);
				CurrentSlot.Cooldown->drawSolidRect(cocos2d::Vec2(-Layout().SlotSize / 2 + Layout().FrameBorder, Layout().SlotSize / 2 - Layout().FrameBorder - CurrentHeight),
					cocos2d::Vec2(Layout().SlotSize / 2 - Layout().FrameBorder, Layout().SlotSize / 2 - Layout().FrameBorder), Paint(Layout().Colors.CooldownShade));
			}
		}

		const int Seconds = CurrentRemaining > Layout().TimerMinSeconds ? static_cast<int>(std::ceil(CurrentRemaining)) : 0;
		if ( Seconds != CurrentSlot.ShownSeconds )
		{
			CurrentSlot.ShownSeconds = Seconds;
			// Only long cooldowns get a number; the fire button just shows the shade.
			CurrentSlot.Timer->setString(Seconds > 1 || (Index > 0 && Seconds > 0) ? std::to_string(Seconds) : "");
		}
	}
}

void Hud::update(float /*InDelta*/)
{
	auto Player = GameContext::Get<ObjectsFactory>().FindObject<Tank>(PlayerName);

	UpdateTopBar();
	UpdatePanel(Player);
	UpdateSlots(Player);
}

} // namespace game::view::cocos
