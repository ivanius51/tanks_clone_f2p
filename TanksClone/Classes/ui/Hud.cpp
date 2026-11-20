#include "ui/Hud.h"

#include <algorithm>
#include <cmath>

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/objects/Tank.h"
#include "game/ObjectsFactory.h"
#include "services/StringsManager.h"
#include "ui/Widgets.h"

namespace tanks::view::cocos
{

namespace
{
	// Top bar
	constexpr float TopPadding = 16.0f;
	constexpr float TopFontSize = 20.0f;
	constexpr float TopSmallFontSize = 18.0f;
	constexpr float WaveX = 300.0f;
	constexpr float EnemiesX = 470.0f;
	constexpr float EnemiesBarX = 640.0f;
	constexpr float EnemiesBarWidth = 130.0f;
	constexpr float EnemiesBarHeight = 12.0f;

	// Player panel
	constexpr float PortraitX = 16.0f;
	constexpr float PortraitSize = 88.0f;
	constexpr float BarsX = 124.0f;
	constexpr float BarsWidth = 300.0f;
	constexpr float HealthY = 74.0f;
	constexpr float HealthHeight = 24.0f;
	constexpr float AmmoY = 46.0f;
	constexpr float AmmoHeight = 20.0f;
	constexpr float ReloadY = 36.0f;
	constexpr float ReloadHeight = 5.0f;
	constexpr float StatsY = 17.0f;

	constexpr float SlotsX = 506.0f;
	constexpr float SlotStep = 88.0f;
	constexpr float SlotSize = 72.0f;
	constexpr float SlotIconSize = 44.0f;
	constexpr size_t MaxSlots = 3;

	const cocos2d::Color4F PanelBackground(0.07f, 0.08f, 0.11f, 1.0f);
	const cocos2d::Color4F PanelBorder(0.35f, 0.4f, 0.5f, 1.0f);
	const cocos2d::Color4F BarBackground(0.16f, 0.17f, 0.21f, 1.0f);
	const cocos2d::Color4F HealthColor(0.3f, 0.8f, 0.3f, 1.0f);
	const cocos2d::Color4F HealthLowColor(0.9f, 0.3f, 0.25f, 1.0f);
	const cocos2d::Color4F AmmoColor(0.95f, 0.7f, 0.2f, 1.0f);
	const cocos2d::Color4F AmmoEmptyColor(0.8f, 0.25f, 0.2f, 1.0f);
	const cocos2d::Color4F ReloadColor(0.9f, 0.9f, 0.9f, 1.0f);
	const cocos2d::Color4F EnemiesColor(0.85f, 0.35f, 0.3f, 1.0f);
	const cocos2d::Color4F SlotFill(0.1f, 0.11f, 0.15f, 1.0f);
	const cocos2d::Color4F SlotBorder(0.5f, 0.55f, 0.65f, 1.0f);
	const cocos2d::Color4F SlotReadyBorder(0.95f, 0.8f, 0.3f, 1.0f);
	const cocos2d::Color4F CooldownShade(0.0f, 0.0f, 0.0f, 0.68f);

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
		InNode->drawSolidRect(Low + cocos2d::Vec2(2, 2), High - cocos2d::Vec2(2, 2), SlotFill);
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
	InNode->drawSolidRect(InArea.origin, InArea.origin + cocos2d::Vec2(InArea.size.width, InArea.size.height), BarBackground);
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
	Top->setPosition(0, BottomHudHeight + GridRows * TileSize);
	addChild(Top);

	auto CurrentBackground = cocos2d::DrawNode::create();
	CurrentBackground->drawSolidRect(cocos2d::Vec2(0, 0), cocos2d::Vec2(DesignWidth, TopHudHeight), PanelBackground);
	CurrentBackground->drawSolidRect(cocos2d::Vec2(0, TopHudHeight - 2), cocos2d::Vec2(DesignWidth, TopHudHeight), PanelBorder);
	Top->addChild(CurrentBackground);

	const float Y = (TopHudHeight - 2) / 2;
	LevelLabel = AddLabel(Top, cocos2d::Vec2(TopPadding, Y), TopFontSize);
	LevelLabel->setColor(widgets::colors::Accent);
	WaveLabel = AddLabel(Top, cocos2d::Vec2(WaveX, Y), TopFontSize);
	EnemiesLabel = AddLabel(Top, cocos2d::Vec2(EnemiesX, Y), TopSmallFontSize);
	EnemiesLabel->setColor(widgets::colors::TextDim);
	EnemiesBar = cocos2d::DrawNode::create();
	Top->addChild(EnemiesBar);
	ScoreLabel = AddLabel(Top, cocos2d::Vec2(DesignWidth - TopPadding, Y), TopFontSize, cocos2d::Vec2(1.0f, 0.5f));
	ScoreLabel->setColor(widgets::colors::Accent);

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

	const std::string CurrentEnemies = Text("HUD_ENEMIES") + " " + std::to_string(Session->Kills) + "/" + std::to_string(Session->TotalEnemies);
	if ( CurrentEnemies != ShownEnemies )
	{
		ShownEnemies = CurrentEnemies;
		EnemiesLabel->setString(CurrentEnemies);
	}
	const int Percent = Session->TotalEnemies > 0 ? 100 * Session->Kills / Session->TotalEnemies : 0;
	if ( Percent != ShownEnemiesPercent )
	{
		ShownEnemiesPercent = Percent;
		DrawBar(EnemiesBar, cocos2d::Rect(EnemiesBarX, (TopHudHeight - 2 - EnemiesBarHeight) / 2, EnemiesBarWidth, EnemiesBarHeight),
			Percent / 100.0f, EnemiesColor);
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

	auto CurrentBackground = cocos2d::DrawNode::create();
	CurrentBackground->drawSolidRect(cocos2d::Vec2(0, 0), cocos2d::Vec2(DesignWidth, BottomHudHeight), PanelBackground);
	CurrentBackground->drawSolidRect(cocos2d::Vec2(0, BottomHudHeight - 2), cocos2d::Vec2(DesignWidth, BottomHudHeight), PanelBorder);
	Panel->addChild(CurrentBackground);

	// Portrait
	const cocos2d::Vec2 PortraitCenter(PortraitX + PortraitSize / 2, (BottomHudHeight - 2) / 2);
	auto CurrentFrame = cocos2d::DrawNode::create();
	CurrentFrame->setPosition(PortraitCenter);
	DrawSlotFrame(CurrentFrame, PortraitSize, PanelBorder);
	Panel->addChild(CurrentFrame);
	auto Portrait = CreateIcon("player_tank", cocos2d::Color3B::WHITE, PortraitSize - 20.0f);
	Portrait->setPosition(PortraitCenter);
	Panel->addChild(Portrait, 1);

	// Bars and stats
	HealthBar = cocos2d::DrawNode::create();
	Panel->addChild(HealthBar);
	AmmoBar = cocos2d::DrawNode::create();
	Panel->addChild(AmmoBar);
	ReloadBar = cocos2d::DrawNode::create();
	Panel->addChild(ReloadBar);

	const float CenterX = BarsX + BarsWidth / 2;
	HealthLabel = AddLabel(Panel, cocos2d::Vec2(CenterX, HealthY + HealthHeight / 2), 17.0f, cocos2d::Vec2(0.5f, 0.5f));
	AmmoLabel = AddLabel(Panel, cocos2d::Vec2(CenterX, AmmoY + AmmoHeight / 2), 15.0f, cocos2d::Vec2(0.5f, 0.5f));
	StatsLabel = AddLabel(Panel, cocos2d::Vec2(BarsX, StatsY), 17.0f);
	StatsLabel->setColor(widgets::colors::TextDim);

	SlotsRoot = Node::create();
	Panel->addChild(SlotsRoot);
}

void Hud::BuildSlots(Tank& InPlayer)
{
	bSlotsBuilt = true;

	const float Y = (BottomHudHeight - 2) / 2;
	for ( size_t Index = 0; Index < std::min(InPlayer.GetAbilityCount(), MaxSlots); ++Index )
	{
		const Ability* CurrentAbility = InPlayer.GetAbility(Index);

		auto CurrentRoot = Node::create();
		CurrentRoot->setPosition(SlotsX + Index * SlotStep, Y);
		SlotsRoot->addChild(CurrentRoot);

		auto CurrentFrame = cocos2d::DrawNode::create();
		DrawSlotFrame(CurrentFrame, SlotSize, PanelBorder);
		CurrentRoot->addChild(CurrentFrame);

		Slot CurrentSlot;
		CurrentSlot.Root = CurrentRoot;
		CurrentSlot.Icon = CreateAbilityIcon(*CurrentAbility, SlotIconSize);
		CurrentSlot.ShownIcon = CurrentAbility->GetIconFrame();
		CurrentSlot.Icon->setPosition(0, 4);
		CurrentRoot->addChild(CurrentSlot.Icon, 1);

		if ( Index == 0 && InPlayer.GetWeaponCount() > 1 )
		{
			auto SwitchKey = widgets::CreateLabel("Q", 15.0f, widgets::colors::Accent);
			SwitchKey->setAnchorPoint(cocos2d::Vec2(1.0f, 1.0f));
			SwitchKey->setPosition(SlotSize / 2 - 4, SlotSize / 2 - 2);
			SwitchKey->enableOutline(cocos2d::Color4B(0, 0, 0, 255), 2);
			CurrentRoot->addChild(SwitchKey, 4);
		}

		CurrentSlot.Cooldown = cocos2d::DrawNode::create();
		CurrentRoot->addChild(CurrentSlot.Cooldown, 2);

		// Slot 0 is the primary ability (fire), the rest are used with E.
		auto Hotkey = widgets::CreateLabel(Index == 0 ? "SPACE" : "E", Index == 0 ? 13.0f : 17.0f, widgets::colors::Accent);
		Hotkey->setAnchorPoint(cocos2d::Vec2(0.0f, 0.0f));
		Hotkey->setPosition(-SlotSize / 2 + 4, -SlotSize / 2 + 2);
		Hotkey->enableOutline(cocos2d::Color4B(0, 0, 0, 255), 2);
		CurrentRoot->addChild(Hotkey, 4);

		CurrentSlot.Timer = widgets::CreateLabel("", 26.0f);
		CurrentSlot.Timer->enableOutline(cocos2d::Color4B(0, 0, 0, 255), 2);
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
		DrawBar(HealthBar, cocos2d::Rect(BarsX, HealthY, BarsWidth, HealthHeight),
			HealthPercent / 100.0f, HealthPercent <= 30 ? HealthLowColor : HealthColor);
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
		DrawBar(AmmoBar, cocos2d::Rect(BarsX, AmmoY, BarsWidth, AmmoHeight), AmmoPercent / 100.0f,
			CurrentAmmo == 0 && !bUnlimited ? AmmoEmptyColor : AmmoColor);
	}

	// Reload
	const int Reload = InPlayer ? static_cast<int>(InPlayer->GetReloadProgress() * 100) : 0;
	if ( Reload != ShownReloadPercent )
	{
		ShownReloadPercent = Reload;
		DrawBar(ReloadBar, cocos2d::Rect(BarsX, ReloadY, BarsWidth, ReloadHeight), Reload / 100.0f, ReloadColor);
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
			CurrentSlot.Icon = CreateAbilityIcon(*CurrentAbility, SlotIconSize);
			CurrentSlot.Icon->setPosition(0, 4);
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
				const float CurrentHeight = SlotSize * (1.0f - Ready / 100.0f);
				CurrentSlot.Cooldown->drawSolidRect(cocos2d::Vec2(-SlotSize / 2 + 2, SlotSize / 2 - 2 - CurrentHeight),
					cocos2d::Vec2(SlotSize / 2 - 2, SlotSize / 2 - 2), CooldownShade);
			}
		}

		const int Seconds = CurrentRemaining > 0.05f ? static_cast<int>(std::ceil(CurrentRemaining)) : 0;
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
