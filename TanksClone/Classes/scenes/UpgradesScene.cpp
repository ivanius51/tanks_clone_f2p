#include "scenes/UpgradesScene.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "game/objects/TankLoader.h"
#include "model/Log.h"
#include "model/NodeNavigator.h"
#include "scenes/MenuScene.h"
#include "scenes/SceneUtils.h"
#include "services/AudioManager.h"
#include "services/ConfigRefs.h"
#include "services/ProfileManager.h"
#include "services/StringsManager.h"
#include "services/UiStyle.h"
#include "services/UpgradeManager.h"
#include "ui/ActionListener.h"
#include "ui/CocosConvert.h"
#include "ui/LayoutWidget.h"
#include "ui/WidgetFactory.h"
#include "ui/Widgets.h"
#include "templates/TContainers.h"

namespace game::view::cocos
{

namespace
{
	constexpr float FullTurn = 6.2831853f;
	constexpr float ArcPieceAngle = 0.08f;
	constexpr int CircleSegments = 40;
	constexpr uint8_t LockedOpacity = 110;
	constexpr float DashLength = 8.0f;
	constexpr float DashGap = 6.0f;
	constexpr float LockBodyWidth = 14.0f;
	constexpr float LockBodyHeight = 10.0f;
	constexpr float LockShackleRadius = 4.5f;
	constexpr float LockOffsetShare = 0.62f;

	const viewmodel::UpgradesLayout& Layout()
	{
		return GameContext::Get<UiStyle>().GetMenu().Upgrades;
	}

	const char* const ShortSuffix = "_SHORT";

	std::string Text(const std::string& InId)
	{
		return GameContext::Get<StringsManager>().GetString(InId);
	}

	UpgradeManager& Shop()
	{
		return GameContext::Get<UpgradeManager>();
	}

	cocos2d::Color4F ToFill(const model::Color& InColor)
	{
		return cocos2d::Color4F(ToCocos3B(InColor));
	}

	void DrawArc(cocos2d::DrawNode* const InNode, const float InRadius, const float InFrom, const float InTo, const float InWidth, const model::Color& InColor)
	{
		const int Pieces = std::max(1, static_cast<int>((InTo - InFrom) / ArcPieceAngle));
		for ( int Piece = 0; Piece < Pieces; ++Piece )
		{
			const float A = InFrom + (InTo - InFrom) * Piece / Pieces;
			const float B = InFrom + (InTo - InFrom) * (Piece + 1) / Pieces;
			InNode->drawSegment(cocos2d::Vec2(std::sin(A), std::cos(A)) * InRadius, cocos2d::Vec2(std::sin(B), std::cos(B)) * InRadius, InWidth / 2, ToFill(InColor));
		}
	}

	void DrawRing(cocos2d::DrawNode* const InNode, const int InLevel, const int InMaxLevel)
	{
		const float Radius = Layout().NodeSize / 2 + Layout().RingGap + Layout().RingWidth / 2;
		const float Segment = FullTurn / std::max(InMaxLevel, 1);
		const float Gap = InMaxLevel > 1 ? Layout().RingSegmentGap : 0.0f;
		for ( int Index = 0; Index < InMaxLevel; ++Index )
		{
			DrawArc(InNode, Radius, Index * Segment + Gap / 2, (Index + 1) * Segment - Gap / 2, Layout().RingWidth,
				Index < InLevel ? Layout().RingFull : Layout().RingEmpty);
		}
	}

	void DrawDisc(cocos2d::DrawNode* const InNode, const model::Color& InEdge)
	{
		const float Radius = Layout().NodeSize / 2;
		InNode->drawSolidCircle(cocos2d::Vec2::ZERO, Radius, 0.0f, CircleSegments, ToFill(InEdge));
		InNode->drawSolidCircle(cocos2d::Vec2::ZERO, Radius - Layout().LinkWidth, 0.0f, CircleSegments, ToFill(Layout().NodeFill));
	}

	void DrawDashedLine(cocos2d::DrawNode* const InNode, const cocos2d::Vec2& InFrom, const cocos2d::Vec2& InTo, const model::Color& InColor)
	{
		const cocos2d::Vec2 Direction = (InTo - InFrom).getNormalized();
		const float Length = InFrom.distance(InTo);
		for ( float Start = 0.0f; Start < Length; Start += DashLength + DashGap )
		{
			InNode->drawSegment(InFrom + Direction * Start, InFrom + Direction * std::min(Start + DashLength, Length), Layout().LinkWidth / 2, ToFill(InColor));
		}
	}

	void DrawDashedRect(cocos2d::DrawNode* const InNode, const cocos2d::Vec2& InFrom, const cocos2d::Vec2& InTo, const model::Color& InColor)
	{
		DrawDashedLine(InNode, InFrom, cocos2d::Vec2(InTo.x, InFrom.y), InColor);
		DrawDashedLine(InNode, cocos2d::Vec2(InTo.x, InFrom.y), InTo, InColor);
		DrawDashedLine(InNode, InTo, cocos2d::Vec2(InFrom.x, InTo.y), InColor);
		DrawDashedLine(InNode, cocos2d::Vec2(InFrom.x, InTo.y), InFrom, InColor);
	}

	void DrawLock(cocos2d::DrawNode* const InNode)
	{
		const float Offset = Layout().NodeSize / 2 * LockOffsetShare;
		const cocos2d::Vec2 Center(Offset, -Offset);
		const cocos2d::Vec2 Half(LockBodyWidth / 2, LockBodyHeight / 2);
		const model::Color& Color = Layout().NodeReady;
		InNode->drawCircle(Center + cocos2d::Vec2(0.0f, Half.y), LockShackleRadius, 0.0f, CircleSegments, false, ToFill(Color));
		InNode->drawSolidRect(Center - Half, Center + Half, ToFill(Color));
	}

	void BackToMenu()
	{
		SwitchScene(MenuScene::CreateScene());
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
	BuildWeaponTabs();
	SelectedWeapon = WeaponTabs.IsEmpty() ? std::string() : WeaponTabs.Front().Branch;
	BuildBlueprint();
	BuildTree();
	BuildTabs();
	BuildPanel();
	AddInput();
	Refresh();
	return true;
}

void UpgradesScene::BuildBlueprint()
{
	auto Paper = cocos2d::DrawNode::create();
	// A wider window than the design size shows past its edges: the paper and the panel reach one design size further each way.
	const float Low = -DesignWidth;
	const float Top = DesignHeight * 2;
	Paper->drawSolidRect(cocos2d::Vec2(Low, Low), cocos2d::Vec2(Layout().PanelX, Top), ToFill(Layout().BackgroundColor));
	const float Step = std::max(Layout().GridStep, 1.0f);
	const int Major = std::max(Layout().GridMajorEvery, 1);
	const int FirstLine = static_cast<int>(std::floor(Low / Step));
	for ( int Line = FirstLine; Line * Step <= Layout().PanelX; ++Line )
	{
		Paper->drawLine(cocos2d::Vec2(Line * Step, Low), cocos2d::Vec2(Line * Step, Top), ToFill(Line % Major ? Layout().GridColor : Layout().GridMajorColor));
	}
	for ( int Line = FirstLine; Line * Step <= Top; ++Line )
	{
		Paper->drawLine(cocos2d::Vec2(Low, Line * Step), cocos2d::Vec2(Layout().PanelX, Line * Step), ToFill(Line % Major ? Layout().GridColor : Layout().GridMajorColor));
	}
	Paper->drawSolidRect(cocos2d::Vec2(Layout().PanelX, Low), cocos2d::Vec2(DesignWidth * 2, Top), ToFill(Layout().PanelColor));
	Paper->drawLine(cocos2d::Vec2(Layout().PanelX, Low), cocos2d::Vec2(Layout().PanelX, Top), ToFill(Layout().EdgeColor));
	addChild(Paper);
	addChild(widgets::CreateControlHints(), 50);

	for ( const viewmodel::UpgradeViewLayout& View : Layout().Views )
	{
		if ( auto Drawing = widgets::CreateFrameSprite(View.Frame) )
		{
			Drawing->setPosition(ToCocos(View.Center));
			Drawing->setScale(View.Width / std::max(Drawing->getContentSize().width, 1.0f));
			addChild(Drawing);
		}
	}

	auto Box = cocos2d::DrawNode::create();
	const cocos2d::Vec2 BoxFrom = ToCocos(Layout().BoxOrigin);
	const cocos2d::Vec2 BoxTo = BoxFrom + ToCocos(Layout().BoxSize);
	Box->drawSolidRect(BoxFrom, BoxTo, ToFill(Layout().BoxColor));
	DrawDashedRect(Box, BoxFrom, BoxTo, Layout().LinkOff);
	addChild(Box, 1);
	BoxTitle = WidgetFactory::Create(ids::widgets::UpgradeBoxTitle);
	BoxTitle->setPosition(BoxFrom.x + Layout().BoxSize.X / 2, BoxTo.y - Layout().NameGap);
	addChild(BoxTitle, 1);
}

UpgradesScene::NodeView UpgradesScene::CreateNodeView(const std::string& InIcon, const std::string& InNameKey, const cocos2d::Vec2& InPosition)
{
	NodeView View;
	View.Root = WidgetFactory::Create(ids::widgets::UpgradeNode);
	const float Outer = (Layout().NodeSize / 2 + Layout().RingGap + Layout().RingWidth) * 2;
	View.Root->SetFixedSize(cocos2d::Size(Outer, Outer));
	View.Root->setPosition(InPosition);
	View.Root->setCascadeOpacityEnabled(true);
	addChild(View.Root, 3);

	View.Frame = cocos2d::DrawNode::create();
	View.Root->FindPart("shape")->addChild(View.Frame);
	View.Root->SetIconFrame("icon", InIcon);
	View.Root->SetText("caption", Text(InNameKey));
	return View;
}

void UpgradesScene::BuildTree()
{
	TArray<model::UpgradeTreeNode> Placed;
	for ( const UpgradeDef& Upgrade : Shop().GetUpgrades() )
	{
		Placed.PushBack({ Upgrade.Id, Upgrade.Branch, Upgrade.Step, Upgrade.Requires });
	}
	Tree.SetBranches(Layout().Branches);
	for ( const FPersistentString Id : Tree.Build(Placed) )
	{
		model::Log::Info("Upgrades: %s has no branch on the tree screen", std::string(Id.View()).c_str());
	}

	Links = cocos2d::DrawNode::create();
	addChild(Links, 2);
	TabLinks = cocos2d::DrawNode::create();
	addChild(TabLinks, 2);
	FocusRing = cocos2d::DrawNode::create();
	addChild(FocusRing, 4);

	for ( int Index = 0; Index < Tree.GetNodeCount(); ++Index )
	{
		const UpgradeDef* Upgrade = nullptr;
		for ( const UpgradeDef& Candidate : Shop().GetUpgrades() )
		{
			if ( Candidate.Id == Tree.GetId(Index) )
			{
				Upgrade = &Candidate;
			}
		}
		Upgrades.PushBack(Upgrade);
		Views.PushBack(CreateNodeView(std::string(Upgrade->Icon.View()), std::string(Upgrade->NameKey.View()) + ShortSuffix, ToCocos(Tree.GetPosition(Index))));
	}
}

void UpgradesScene::BuildWeaponTabs()
{
	const ConfigReader& Reader = GameContext::Get<ConfigReader>();
	const TankDef Player = TankLoader::ReadTank(Reader.GetConfig(config_refs::ResolveId(ids::configs::PlayerTank)));
	for ( const WeaponDef& Weapon : Player.Weapons )
	{
		const auto& Branches = Layout().Branches;
		const bool bHasBranch = std::any_of(Branches.begin(), Branches.end(), [&Weapon](const model::UpgradeBranch& InBranch) { return InBranch.Id == Weapon.Id.View(); });
		if ( bHasBranch )
		{
			WeaponTabs.PushBack({ std::string(Weapon.Id.View()), std::string(Weapon.NameKey.View()), std::string(Weapon.Icon.View()) });
		}
	}
}

void UpgradesScene::BuildTabs()
{
	for ( const viewmodel::UpgradeWeaponTab& Tab : WeaponTabs )
	{
		model::Vec2f Position;
		for ( const model::UpgradeBranch& Branch : Layout().Branches )
		{
			if ( Branch.Id == Tab.Branch )
			{
				Position = Branch.Origin;
			}
		}
		NodeView View = CreateNodeView(Tab.Icon, Tab.NameKey, ToCocos(Position));
		View.Root->SetPartVisible("badge", false);
		TabHomes.PushBack(ToCocos(Position));
		Tabs.PushBack(View);
	}
	for ( int Tab = 0; Tab < static_cast<int>(Tabs.Size()); ++Tab )
	{
		Tabs[Tab].Root->setPosition(GetTabTarget(Tab));
		Tabs[Tab].Root->setScale(WeaponTabs[Tab].Branch == SelectedWeapon ? 1.0f : 0.0f);
	}
}

cocos2d::Vec2 UpgradesScene::GetTabTarget(const int InTab) const
{
	return bWeaponsOpen ? TabHomes[InTab] : ToCocos(Layout().TurretPoint);
}

void UpgradesScene::SetWeaponsOpen(const bool bInOpen)
{
	bWeaponsOpen = bInOpen;
	const float Seconds = Layout().WeaponsOpenSeconds;
	WeaponsMoveLeft = Seconds + Layout().WeaponsStaggerSeconds * static_cast<float>(Tabs.Size());
	scheduleUpdate();
	for ( int Tab = 0; Tab < static_cast<int>(Tabs.Size()); ++Tab )
	{
		cocos2d::Node* const Root = Tabs[Tab].Root;
		const bool bSelected = WeaponTabs[Tab].Branch == SelectedWeapon;
		Root->stopAllActions();
		Root->setLocalZOrder(bSelected ? 3 : 2);
		cocos2d::FiniteTimeAction* const Move = bInOpen
			? static_cast<cocos2d::FiniteTimeAction*>(cocos2d::EaseBackOut::create(cocos2d::MoveTo::create(Seconds, GetTabTarget(Tab))))
			: static_cast<cocos2d::FiniteTimeAction*>(cocos2d::EaseSineIn::create(cocos2d::MoveTo::create(Seconds, GetTabTarget(Tab))));
		Root->runAction(cocos2d::Sequence::create(cocos2d::DelayTime::create(Layout().WeaponsStaggerSeconds * Tab),
			cocos2d::Spawn::create(Move, cocos2d::ScaleTo::create(Seconds, bInOpen || bSelected ? 1.0f : 0.0f), nullptr), nullptr));
	}
	Refresh();
}

void UpgradesScene::BuildPanel()
{
	LayoutWidget* const Panel = WidgetFactory::Create(ids::widgets::UpgradePanel);
	Panel->SetFixedSize(cocos2d::Size(Layout().PanelWidth, DesignHeight));
	Panel->setPosition(Layout().PanelX + Layout().PanelWidth / 2, DesignHeight / 2);
	addChild(Panel);
	Texts = Panel->FindWidget("texts");
	Texts->SetFixedSize(cocos2d::Size(Layout().PanelWidth, 0.0f));
	Texts->SetText("title", Text("UPGRADES_TITLE"));

	const cocos2d::Size ButtonSize = widgets::ScaleUi(cocos2d::Size(Layout().ButtonWidth, Layout().ButtonHeight));
	Buy = widgets::Button::create(Text("UPGRADE_BUY"), ButtonSize, [this]() { Confirm(); });
	auto Back = widgets::Button::create(Text("MENU_BACK"), ButtonSize, []() { BackToMenu(); });
	LayoutWidget* const Buttons = Panel->FindWidget("buttons");
	Buttons->AppendNode(Buy);
	Buttons->AppendNode(Back);
}

void UpgradesScene::AddInput()
{
	AddActionHandler(this, [this](const model::InputAction InAction)
	{
		switch ( InAction )
		{
			case model::InputAction::UiUp:
				Move(model::Vec2f(0.0f, 1.0f));
				break;
			case model::InputAction::UiDown:
				Move(model::Vec2f(0.0f, -1.0f));
				break;
			case model::InputAction::UiLeft:
				Move(model::Vec2f(-1.0f, 0.0f));
				break;
			case model::InputAction::UiRight:
				Move(model::Vec2f(1.0f, 0.0f));
				break;
			case model::InputAction::UiConfirm:
				Confirm();
				break;
			default:
				break;
		}
	});
	widgets::AddBackKeyHandler(this, []() { BackToMenu(); });

	auto Touch = cocos2d::EventListenerTouchOneByOne::create();
	Touch->onTouchBegan = [this](cocos2d::Touch* const InTouch, cocos2d::Event*)
	{
		const int Index = FindNodeAt(convertToNodeSpace(InTouch->getLocation()));
		if ( Index < 0 )
		{
			return false;
		}
		if ( Index == Focused )
		{
			Confirm();
		}
		else
		{
			SetFocus(Index);
			if ( IsTab(Index) )
			{
				SetWeaponsOpen(false);
			}
		}
		return true;
	};
	getEventDispatcher()->addEventListenerWithSceneGraphPriority(Touch, this);
}

void UpgradesScene::Refresh()
{
	Links->clear();
	DrawTabLinks();
	for ( int Tab = 0; Tab < static_cast<int>(Tabs.Size()); ++Tab )
	{
		RefreshTab(Tab);
	}
	for ( int Index = 0; Index < Tree.GetNodeCount(); ++Index )
	{
		RefreshNode(Index);
	}
	for ( const viewmodel::UpgradeWeaponTab& Tab : WeaponTabs )
	{
		if ( Tab.Branch == SelectedWeapon )
		{
			BoxTitle->SetText("text", Text(Tab.NameKey));
		}
	}
	Texts->SetText("coins", Text("UPGRADES_COINS") + ": " + std::to_string(GameContext::Get<ProfileManager>().Get().Coins));
	SetFocus(Focused);
}

void UpgradesScene::RefreshNode(const int InIndex)
{
	const UpgradeDef& Upgrade = *Upgrades[InIndex];
	NodeView& View = Views[InIndex];
	const bool bShown = IsShown(InIndex);
	View.Root->setVisible(bShown);
	if ( !bShown )
	{
		return;
	}

	const int CurrentLevel = Shop().GetLevel(Upgrade.Id.View());
	const int Cost = Shop().GetNextCost(Upgrade.Id.View());
	const bool bIsOpen = Shop().IsOpen(Upgrade.Id.View());
	const model::Color& Edge = Cost < 0 ? Layout().NodeDone : (bIsOpen ? Layout().NodeReady : Layout().NodeLocked);
	View.Frame->clear();
	DrawDisc(View.Frame, Edge);
	DrawRing(View.Frame, CurrentLevel, Upgrade.MaxLevel);
	if ( !bIsOpen && CurrentLevel == 0 )
	{
		DrawLock(View.Frame);
	}
	View.Root->setOpacity(bIsOpen || CurrentLevel > 0 ? 255 : LockedOpacity);

	View.Root->SetPartVisible("badge", Cost >= 0 && bIsOpen);
	View.Root->FindWidget("badge")->SetText("price", "{icon:bonus_coin} " + std::to_string(Cost));

	if ( IsWeaponBranch(std::string(Upgrade.Branch.View())) && Upgrade.Step == 0 )
	{
		return;
	}
	const cocos2d::Vec2 From = ToCocos(Tree.GetLinkStart(InIndex));
	Links->drawSegment(From, View.Root->getPosition(), Layout().LinkWidth / 2, ToFill(CurrentLevel > 0 ? Layout().LinkOn : Layout().LinkOff));
}

void UpgradesScene::RefreshTab(const int InTab)
{
	const std::string& Branch = WeaponTabs[InTab].Branch;
	NodeView& View = Tabs[InTab];
	const bool bSelected = Branch == SelectedWeapon;
	const bool bUnlocked = IsWeaponUnlocked(Branch);
	View.Frame->clear();
	DrawDisc(View.Frame, bSelected ? Layout().NodeDone : (bUnlocked ? Layout().NodeReady : Layout().NodeLocked));
	View.Root->setOpacity(bUnlocked || bSelected ? 255 : LockedOpacity);
}

void UpgradesScene::DrawTabLinks()
{
	TabLinks->clear();
	for ( int Tab = 0; Tab < static_cast<int>(Tabs.Size()); ++Tab )
	{
		const cocos2d::Node* const Root = Tabs[Tab].Root;
		const bool bSelected = WeaponTabs[Tab].Branch == SelectedWeapon;
		if ( Root->getScale() > 0.0f && (bWeaponsOpen || WeaponsMoveLeft > 0.0f) )
		{
			TabLinks->drawSegment(Root->getPosition(), ToCocos(Layout().TurretPoint), Layout().LinkWidth / 2, ToFill(bSelected ? Layout().LinkOn : Layout().LinkOff));
		}
	}
}

void UpgradesScene::update(const float InDelta)
{
	Scene::update(InDelta);
	WeaponsMoveLeft = std::max(WeaponsMoveLeft - InDelta, 0.0f);
	DrawTabLinks();
	if ( WeaponsMoveLeft <= 0.0f )
	{
		unscheduleUpdate();
	}
}

std::string UpgradesScene::DescribeEffect(const UpgradeDef& InUpgrade) const
{
	const int Level = Shop().GetLevel(InUpgrade.Id.View());
	const auto Format = [&InUpgrade](const int InLevels)
	{
		if ( !InUpgrade.Modifiers.IsEmpty() )
		{
			const StatModifier& Modifier = InUpgrade.Modifiers[0];
			if ( Modifier.Add != 0.0f )
			{
				return cocos2d::StringUtils::format("+%g", Modifier.Add * static_cast<float>(InLevels));
			}
			return cocos2d::StringUtils::format("+%d%%", static_cast<int>(std::lround((std::pow(Modifier.Mul, InLevels) - 1.0f) * 100.0f)));
		}
		if ( InUpgrade.WeaponDamage != 0 )
		{
			return cocos2d::StringUtils::format("+%d", InUpgrade.WeaponDamage * InLevels);
		}
		if ( InUpgrade.WeaponReload != 1.0f )
		{
			return cocos2d::StringUtils::format("-%d%%", static_cast<int>(std::lround((1.0f - std::pow(InUpgrade.WeaponReload, InLevels)) * 100.0f)));
		}
		return std::string();
	};
	const std::string AtMax = Format(InUpgrade.MaxLevel);
	if ( AtMax.empty() )
	{
		return std::string();
	}
	return "\n" + Text("UPGRADE_NOW") + ": " + (Level > 0 ? Format(Level) : std::string("0")) + "\n" + Text("UPGRADE_AT_MAX") + ": " + AtMax;
}

void UpgradesScene::RefreshPanel()
{
	if ( IsTab(Focused) )
	{
		const viewmodel::UpgradeWeaponTab& Tab = WeaponTabs[Focused - Tree.GetNodeCount()];
		Texts->SetIconFrame("icon", Tab.Icon);
		Texts->SetPartFraction("progress", 0.0f);
		Texts->SetText("name", Text(Tab.NameKey));
		Texts->SetText("info", std::string());
		Texts->SetText("level", std::string());
		Texts->SetText("need", std::string());
		Buy->SetText(Text("UPGRADE_BUY"));
		Buy->SetEnabled(false);
		return;
	}
	const UpgradeDef& Upgrade = *Upgrades[Focused];
	const int Cost = Shop().GetNextCost(Upgrade.Id.View());
	const bool bIsOpen = Shop().IsOpen(Upgrade.Id.View());
	Texts->SetIconFrame("icon", std::string(Upgrade.Icon.View()));
	Texts->SetPartFraction("progress", static_cast<float>(Shop().GetLevel(Upgrade.Id.View())) / static_cast<float>(std::max(1, Upgrade.MaxLevel)));
	Texts->SetText("name", Text(std::string(Upgrade.NameKey.View())));
	Texts->SetText("info", Text(std::string(Upgrade.NameKey.View()) + "_INFO") + DescribeEffect(Upgrade));
	Texts->SetText("level", Text("UPGRADE_LEVEL") + " " + std::to_string(Shop().GetLevel(Upgrade.Id.View())) + " / " + std::to_string(Upgrade.MaxLevel));
	Texts->SetText("need", bIsOpen ? std::string() : GetRequirementText(Upgrade));
	Buy->SetText(Cost < 0 ? Text("UPGRADE_MAX") : (bIsOpen ? Text("UPGRADE_BUY") + "  {icon:bonus_coin} " + std::to_string(Cost) : Text("UPGRADE_LOCKED")));
	Buy->SetEnabled(Shop().CanBuy(Upgrade.Id.View()));
}

void UpgradesScene::SetFocus(const int InIndex)
{
	if ( InIndex < 0 || InIndex >= GetFocusCount() )
	{
		return;
	}
	Focused = InIndex;
	if ( bWeaponsOpen && !IsTab(Focused) )
	{
		SetWeaponsOpen(false);
	}
	if ( IsTab(Focused) && WeaponTabs[Focused - Tree.GetNodeCount()].Branch != SelectedWeapon )
	{
		SelectedWeapon = WeaponTabs[Focused - Tree.GetNodeCount()].Branch;
		Refresh();
		return;
	}
	FocusRing->clear();
	FocusRing->setLineWidth(Layout().FocusWidth);
	FocusRing->drawCircle(GetFocusPosition(Focused), Layout().NodeSize / 2 + Layout().FocusGap, 0.0f, CircleSegments, false, ToFill(Layout().FocusColor));
	RefreshPanel();
}

void UpgradesScene::Move(const model::Vec2f& InDirection)
{
	TArray<model::Vec2f> Positions;
	TArray<uint8_t> Selectable;
	for ( int Index = 0; Index < GetFocusCount(); ++Index )
	{
		const cocos2d::Vec2 Position = GetFocusPosition(Index);
		Positions.EmplaceBack(Position.x, Position.y);
		Selectable.PushBack(IsShown(Index) ? 1 : 0);
	}
	SetFocus(model::NodeNavigator::FindNext(Positions, Selectable, Focused, InDirection));
}

void UpgradesScene::Confirm()
{
	if ( IsTab(Focused) && !bWeaponsOpen )
	{
		SetWeaponsOpen(true);
		return;
	}
	if ( IsTab(Focused) )
	{
		SetWeaponsOpen(false);
		for ( int Index = 0; Index < Tree.GetNodeCount(); ++Index )
		{
			if ( IsShown(Index) && IsWeaponBranch(std::string(Upgrades[Index]->Branch.View())) && Upgrades[Index]->Step == 0 )
			{
				SetFocus(Index);
			}
		}
		return;
	}
	if ( Focused < 0 || Focused >= Tree.GetNodeCount() || !Shop().Buy(Upgrades[Focused]->Id.View()) )
	{
		return;
	}
	GameContext::Get<AudioManager>().Play(ids::sounds::BonusPickup);
	Refresh();
}

bool UpgradesScene::IsShown(const int InIndex) const
{
	if ( IsTab(InIndex) )
	{
		return bWeaponsOpen || WeaponTabs[InIndex - Tree.GetNodeCount()].Branch == SelectedWeapon;
	}
	const std::string Branch(Upgrades[InIndex]->Branch.View());
	return !Shop().IsHidden(Upgrades[InIndex]->Id.View()) && (!IsWeaponBranch(Branch) || Branch == SelectedWeapon);
}

bool UpgradesScene::IsTab(const int InIndex) const
{
	return InIndex >= Tree.GetNodeCount() && InIndex < GetFocusCount();
}

bool UpgradesScene::IsWeaponBranch(const std::string& InBranch) const
{
	const auto& Weapons = WeaponTabs;
	return std::any_of(Weapons.begin(), Weapons.end(), [&InBranch](const viewmodel::UpgradeWeaponTab& InTab) { return InTab.Branch == InBranch; });
}

bool UpgradesScene::IsWeaponUnlocked(const std::string& InBranch) const
{
	for ( const UpgradeDef& Upgrade : Shop().GetUpgrades() )
	{
		if ( Upgrade.Branch == InBranch && !Upgrade.UnlockWeapon.IsEmpty() && Shop().GetLevel(Upgrade.Id.View()) == 0 )
		{
			return false;
		}
	}
	return true;
}

int UpgradesScene::GetFocusCount() const
{
	return Tree.GetNodeCount() + static_cast<int>(Tabs.Size());
}

cocos2d::Vec2 UpgradesScene::GetFocusPosition(const int InIndex) const
{
	return IsTab(InIndex) ? GetTabTarget(InIndex - Tree.GetNodeCount()) : Views[InIndex].Root->getPosition();
}

int UpgradesScene::FindNodeAt(const cocos2d::Vec2& InPoint) const
{
	const float Radius = Layout().NodeSize / 2;
	for ( int Index = 0; Index < GetFocusCount(); ++Index )
	{
		if ( IsShown(Index) && InPoint.distance(GetFocusPosition(Index)) <= Radius )
		{
			return Index;
		}
	}
	return -1;
}

std::string UpgradesScene::GetRequirementText(const UpgradeDef& InUpgrade) const
{
	const UpgradeDef* const Requirement = Shop().GetRequirement(InUpgrade.Id.View());
	if ( !Requirement )
	{
		return std::string();
	}
	const std::string RequiredLevel = InUpgrade.RequiresLevel > 1 ? " " + Text("UPGRADE_LEVEL") + " " + std::to_string(InUpgrade.RequiresLevel) : std::string();
	return Text("UPGRADE_NEEDS") + ": " + Text(std::string(Requirement->NameKey.View())) + RequiredLevel;
}

} // namespace game::view::cocos
