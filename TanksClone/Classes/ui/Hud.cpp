#include "ui/Hud.h"

#include <algorithm>
#include <cmath>

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/objects/Tank.h"
#include "game/ObjectsFactory.h"
#include "services/InputManager.h"
#include "services/StringsManager.h"
#include "services/UiStyle.h"
#include "ui/CocosConvert.h"
#include "ui/ScreenFit.h"
#include "ui/Widgets.h"

namespace tanks::view::cocos
{

namespace
{
	const viewmodel::HudLayout& Layout()
	{
		return GameContext::Get().GetManager<UiStyle>().GetHud();
	}

	cocos2d::Color4F Paint(const model::Color& InColor)
	{
		return cocos2d::Color4F(ToCocos(InColor));
	}

	// Top bar

	// Player panel



	std::string Text(const std::string& InId)
	{
		return GameContext::Get().GetManager<StringsManager>().GetString(InId);
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

void Hud::DrawBar(cocos2d::DrawNode* const InNode, const cocos2d::Rect& InArea, const float InFraction, const cocos2d::Color4F& InColor)
{
	InNode->clear();
	InNode->drawSolidRect(InArea.origin, InArea.origin + cocos2d::Vec2(InArea.size.width, InArea.size.height), Paint(Layout().Colors.BarBackground));
	if ( InFraction > 0.0f )
	{
		InNode->drawSolidRect(InArea.origin, InArea.origin + cocos2d::Vec2(InArea.size.width * std::min(1.0f, InFraction), InArea.size.height), InColor);
	}
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
	EnemiesBar = cocos2d::DrawNode::create();
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
		DrawBar(EnemiesBar, cocos2d::Rect(Layout().EnemiesBarX, (TopHudHeight - Layout().BandBorder - Layout().EnemiesBarHeight) / 2, Layout().EnemiesBarWidth, Layout().EnemiesBarHeight),
			Percent / 100.0f, Paint(Layout().Colors.EnemiesColor));
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
	HealthBar = cocos2d::DrawNode::create();
	Panel->addChild(HealthBar);
	AmmoBar = cocos2d::DrawNode::create();
	Panel->addChild(AmmoBar);
	ReloadBar = cocos2d::DrawNode::create();
	Panel->addChild(ReloadBar);

	const float CenterX = Layout().BarsX + Layout().BarsWidth / 2;
	HealthLabel = AddLabel(Panel, cocos2d::Vec2(CenterX, Layout().HealthY + Layout().HealthHeight / 2), Layout().HealthFontSize, cocos2d::Vec2(0.5f, 0.5f));
	AmmoLabel = AddLabel(Panel, cocos2d::Vec2(CenterX, Layout().AmmoY + Layout().AmmoHeight / 2), Layout().AmmoFontSize, cocos2d::Vec2(0.5f, 0.5f));
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

	const float Y = (BottomHudHeight - Layout().BandBorder) / 2;
	const InputManager& Input = GameContext::Get().GetManager<InputManager>();
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
			auto SwitchKey = widgets::CreateLabel(SwitchKeyText, Layout().SwitchKeyFontSize, widgets::colors::Accent());
			SwitchKey->setAnchorPoint(cocos2d::Vec2(1.0f, 1.0f));
			SwitchKey->setPosition(Layout().SlotSize / 2 - Layout().KeyInsetX, Layout().SlotSize / 2 - Layout().KeyInsetY);
			widgets::AddOutline(SwitchKey);
			CurrentRoot->addChild(SwitchKey, 4);
		}

		CurrentSlot.Cooldown = cocos2d::DrawNode::create();
		CurrentRoot->addChild(CurrentSlot.Cooldown, 2);

		// Slot 0 is the primary ability (fire), the rest are used with the ability action.
		const std::string& HotkeyText = Index == 0 ? FireKey : AbilityKey;
		auto Hotkey = widgets::CreateLabel(HotkeyText, HotkeyText.size() > 1 ? Layout().LongKeyFontSize : Layout().KeyFontSize, widgets::colors::Accent());
		Hotkey->setAnchorPoint(cocos2d::Vec2(0.0f, 0.0f));
		Hotkey->setPosition(-Layout().SlotSize / 2 + Layout().KeyInsetX, -Layout().SlotSize / 2 + Layout().KeyInsetY);
		widgets::AddOutline(Hotkey);
		CurrentRoot->addChild(Hotkey, 4);

		CurrentSlot.Timer = widgets::CreateLabel("", Layout().TimerFontSize);
		widgets::AddOutline(CurrentSlot.Timer);
		CurrentRoot->addChild(CurrentSlot.Timer, 3);

		Slots.push_back(CurrentSlot);
	}
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
		HealthLabel->setString(HealthText);
	}
	if ( HealthPercent != ShownHealthPercent )
	{
		ShownHealthPercent = HealthPercent;
		DrawBar(HealthBar, cocos2d::Rect(Layout().BarsX, Layout().HealthY, Layout().BarsWidth, Layout().HealthHeight),
			HealthPercent / 100.0f, HealthPercent <= Layout().HealthLowPercent ? Paint(Layout().Colors.HealthLowColor) : Paint(Layout().Colors.HealthColor));
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
		AmmoLabel->setString(AmmoText);
	}
	const int AmmoPercent = bUnlimited ? 100 : (MaxAmmo > 0 ? 100 * CurrentAmmo / MaxAmmo : 0);
	if ( AmmoPercent != ShownAmmoPercent )
	{
		ShownAmmoPercent = AmmoPercent;
		DrawBar(AmmoBar, cocos2d::Rect(Layout().BarsX, Layout().AmmoY, Layout().BarsWidth, Layout().AmmoHeight), AmmoPercent / 100.0f,
			CurrentAmmo == 0 && !bUnlimited ? Paint(Layout().Colors.AmmoEmptyColor) : Paint(Layout().Colors.AmmoColor));
	}

	// Reload
	const int Reload = InPlayer ? static_cast<int>(InPlayer->GetReloadProgress() * 100) : 0;
	if ( Reload != ShownReloadPercent )
	{
		ShownReloadPercent = Reload;
		DrawBar(ReloadBar, cocos2d::Rect(Layout().BarsX, Layout().ReloadY, Layout().BarsWidth, Layout().ReloadHeight), Reload / 100.0f, Paint(Layout().Colors.ReloadColor));
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
	if ( InPlayer && !bSlotsBuilt )
	{
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
	auto Player = GameContext::Get().GetManager<ObjectsFactory>().FindObject<Tank>(PlayerName);

	UpdateTopBar();
	UpdatePanel(Player);
	UpdateSlots(Player);
}

} // namespace tanks::view::cocos
