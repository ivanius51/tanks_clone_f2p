#include "scenes/UpgradesScene.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "model/NodeNavigator.h"
#include "scenes/MenuScene.h"
#include "scenes/SceneUtils.h"
#include "services/AudioManager.h"
#include "services/ProfileManager.h"
#include "services/StringsManager.h"
#include "services/UiStyle.h"
#include "services/UpgradeManager.h"
#include "ui/ActionListener.h"
#include "ui/CocosConvert.h"
#include "ui/Widgets.h"

namespace tanks::view::cocos
{

namespace
{
	constexpr float FullTurn = 6.2831853f;
	constexpr float ArcPieceAngle = 0.08f;
	constexpr int CircleSegments = 40;
	constexpr uint8_t LockedOpacity = 110;

	const viewmodel::UpgradesLayout& Layout()
	{
		return GameContext::Get().GetManager<UiStyle>().GetMenu().Upgrades;
	}

	std::string Text(const std::string& InId)
	{
		return GameContext::Get().GetManager<StringsManager>().GetString(InId);
	}

	UpgradeManager& Shop()
	{
		return GameContext::Get().GetManager<UpgradeManager>();
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
	SelectedWeapon = Layout().SelectedWeapon;
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
	Paper->drawSolidRect(cocos2d::Vec2::ZERO, cocos2d::Vec2(DesignWidth, DesignHeight), ToFill(Layout().BackgroundColor));
	const float Step = std::max(Layout().GridStep, 1.0f);
	const int Major = std::max(Layout().GridMajorEvery, 1);
	for ( int Line = 0; Line * Step <= Layout().PanelX; ++Line )
	{
		Paper->drawLine(cocos2d::Vec2(Line * Step, 0.0f), cocos2d::Vec2(Line * Step, DesignHeight), ToFill(Line % Major ? Layout().GridColor : Layout().GridMajorColor));
	}
	for ( int Line = 0; Line * Step <= DesignHeight; ++Line )
	{
		Paper->drawLine(cocos2d::Vec2(0.0f, Line * Step), cocos2d::Vec2(Layout().PanelX, Line * Step), ToFill(Line % Major ? Layout().GridColor : Layout().GridMajorColor));
	}
	Paper->drawSolidRect(cocos2d::Vec2(Layout().PanelX, 0.0f), cocos2d::Vec2(DesignWidth, DesignHeight), ToFill(Layout().PanelColor));
	Paper->drawLine(cocos2d::Vec2(Layout().PanelX, 0.0f), cocos2d::Vec2(Layout().PanelX, DesignHeight), ToFill(Layout().EdgeColor));
	addChild(Paper);

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
	Box->drawRect(BoxFrom, BoxTo, ToFill(Layout().LinkOn));
	addChild(Box, 1);
	BoxTitle = widgets::CreateLabel("", Layout().NodeFontSize, widgets::colors::Accent());
	BoxTitle->setPosition(BoxFrom.x + Layout().BoxSize.X / 2, BoxTo.y - Layout().NameGap);
	addChild(BoxTitle, 1);
}

UpgradesScene::NodeView UpgradesScene::CreateNodeView(const std::string& InIcon, const std::string& InNameKey, const cocos2d::Vec2& InPosition)
{
	const float Half = Layout().NodeSize / 2;
	NodeView View;
	View.Root = cocos2d::Node::create();
	View.Root->setPosition(InPosition);
	View.Root->setCascadeOpacityEnabled(true);
	addChild(View.Root, 3);

	View.Frame = cocos2d::DrawNode::create();
	View.Root->addChild(View.Frame);
	if ( auto Icon = widgets::CreateFrameSprite(InIcon) )
	{
		const cocos2d::Size IconSize = Icon->getContentSize();
		Icon->setScale(Layout().IconSize / std::max(std::max(IconSize.width, IconSize.height), 1.0f));
		View.Root->addChild(Icon, 1);
	}

	auto Caption = widgets::CreateLabel(Text(InNameKey), Layout().NodeFontSize);
	Caption->setPosition(0.0f, -Half - Layout().RingGap - Layout().RingWidth - Layout().NameGap);
	View.Root->addChild(Caption, 1);

	View.Badge = cocos2d::Node::create();
	View.Badge->setPosition(0.0f, Half + Layout().RingGap + Layout().RingWidth + Layout().BadgeSize.Y / 2);
	auto BadgeBack = cocos2d::DrawNode::create();
	const float Round = Layout().BadgeSize.Y / 2;
	const float Straight = std::max(Layout().BadgeSize.X / 2 - Round, 0.0f);
	BadgeBack->drawSolidRect(cocos2d::Vec2(-Straight, -Round), cocos2d::Vec2(Straight, Round), ToFill(Layout().BadgeColor));
	BadgeBack->drawSolidCircle(cocos2d::Vec2(-Straight, 0.0f), Round, 0.0f, CircleSegments / 2, ToFill(Layout().BadgeColor));
	BadgeBack->drawSolidCircle(cocos2d::Vec2(Straight, 0.0f), Round, 0.0f, CircleSegments / 2, ToFill(Layout().BadgeColor));
	View.Badge->addChild(BadgeBack);
	View.Price = widgets::CreateLabel("", Layout().PriceFontSize);
	View.Badge->addChild(View.Price, 1);
	View.Root->addChild(View.Badge, 2);
	return View;
}

void UpgradesScene::BuildTree()
{
	std::vector<model::UpgradeTreeNode> Placed;
	for ( const UpgradeDef& Upgrade : Shop().GetUpgrades() )
	{
		Placed.push_back({ Upgrade.Id, Upgrade.Branch, Upgrade.Step, Upgrade.Requires });
	}
	Tree.SetBranches(Layout().Branches);
	for ( const std::string& Id : Tree.Build(Placed) )
	{
		cocos2d::log("Upgrades: %s has no branch on the tree screen", Id.c_str());
	}

	Links = cocos2d::DrawNode::create();
	addChild(Links, 2);
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
		Upgrades.push_back(Upgrade);
		Views.push_back(CreateNodeView(Upgrade->Icon, Upgrade->NameKey, ToCocos(Tree.GetPosition(Index))));
	}
}

void UpgradesScene::BuildTabs()
{
	for ( const viewmodel::UpgradeWeaponTab& Tab : Layout().Weapons )
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
		View.Badge->setVisible(false);
		Tabs.push_back(View);
	}
}

void UpgradesScene::BuildPanel()
{
	const float CenterX = Layout().PanelX + Layout().PanelWidth / 2;
	auto Title = widgets::CreateLabel(Text("UPGRADES_TITLE"), Layout().TitleFontSize, widgets::colors::Accent(), true);
	Title->setPosition(CenterX, DesignHeight - Layout().TitleTop);
	addChild(Title);

	Coins = widgets::CreateLabel("", Layout().NameFontSize, widgets::colors::Accent());
	Coins->setPosition(CenterX, DesignHeight - Layout().CoinsTop);
	addChild(Coins);

	Name = widgets::CreateLabel("", Layout().NameFontSize);
	Name->setPosition(CenterX, DesignHeight - Layout().NameTop);
	addChild(Name);

	Info = widgets::CreateLabel("", Layout().InfoFontSize, widgets::colors::TextDim());
	Info->setDimensions(Layout().PanelWidth, 0.0f);
	Info->setAlignment(cocos2d::TextHAlignment::CENTER);
	Info->setAnchorPoint(cocos2d::Vec2(0.5f, 1.0f));
	Info->setPosition(CenterX, DesignHeight - Layout().InfoTop);
	addChild(Info);

	Level = widgets::CreateLabel("", Layout().InfoFontSize);
	Level->setPosition(CenterX, DesignHeight - Layout().LevelTop);
	addChild(Level);

	Need = widgets::CreateLabel("", Layout().InfoFontSize, widgets::colors::TextDim());
	Need->setDimensions(Layout().PanelWidth, 0.0f);
	Need->setAlignment(cocos2d::TextHAlignment::CENTER);
	Need->setAnchorPoint(cocos2d::Vec2(0.5f, 1.0f));
	Need->setPosition(CenterX, DesignHeight - Layout().NeedTop);
	addChild(Need);

	const cocos2d::Size ButtonSize = widgets::ScaleUi(cocos2d::Size(Layout().ButtonWidth, Layout().ButtonHeight));
	Buy = widgets::Button::create(Text("UPGRADE_BUY"), ButtonSize, [this]() { Confirm(); });
	Buy->setPosition(CenterX, Layout().BuyBottom);
	addChild(Buy, 1);

	auto Back = widgets::Button::create(Text("MENU_BACK"), ButtonSize, []() { BackToMenu(); });
	Back->setPosition(CenterX, Layout().BackBottom);
	addChild(Back, 1);
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
		}
		return true;
	};
	getEventDispatcher()->addEventListenerWithSceneGraphPriority(Touch, this);
}

void UpgradesScene::Refresh()
{
	Links->clear();
	for ( int Tab = 0; Tab < static_cast<int>(Tabs.size()); ++Tab )
	{
		RefreshTab(Tab);
	}
	for ( int Index = 0; Index < Tree.GetNodeCount(); ++Index )
	{
		RefreshNode(Index);
	}
	for ( const viewmodel::UpgradeWeaponTab& Tab : Layout().Weapons )
	{
		if ( Tab.Branch == SelectedWeapon )
		{
			BoxTitle->setString(Text(Tab.NameKey));
		}
	}
	Coins->setString(Text("UPGRADES_COINS") + ": " + std::to_string(GameContext::Get().GetManager<ProfileManager>().Get().Coins));
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

	const int CurrentLevel = Shop().GetLevel(Upgrade.Id);
	const int Cost = Shop().GetNextCost(Upgrade.Id);
	const bool bIsOpen = Shop().IsOpen(Upgrade.Id);
	const model::Color& Edge = Cost < 0 ? Layout().NodeDone : (bIsOpen ? Layout().NodeReady : Layout().NodeLocked);
	View.Frame->clear();
	DrawDisc(View.Frame, Edge);
	DrawRing(View.Frame, CurrentLevel, Upgrade.MaxLevel);
	View.Root->setOpacity(bIsOpen || CurrentLevel > 0 ? 255 : LockedOpacity);

	View.Badge->setVisible(Cost >= 0 && bIsOpen);
	View.Price->setString(std::to_string(Cost));

	if ( IsWeaponBranch(Upgrade.Branch) && Upgrade.Step == 0 )
	{
		return;
	}
	const cocos2d::Vec2 From = ToCocos(Tree.GetLinkStart(InIndex));
	Links->drawSegment(From, View.Root->getPosition(), Layout().LinkWidth / 2, ToFill(CurrentLevel > 0 ? Layout().LinkOn : Layout().LinkOff));
}

void UpgradesScene::RefreshTab(const int InTab)
{
	const std::string& Branch = Layout().Weapons[InTab].Branch;
	NodeView& View = Tabs[InTab];
	const bool bSelected = Branch == SelectedWeapon;
	const bool bUnlocked = IsWeaponUnlocked(Branch);
	View.Frame->clear();
	DrawDisc(View.Frame, bSelected ? Layout().NodeDone : (bUnlocked ? Layout().NodeReady : Layout().NodeLocked));
	View.Root->setOpacity(bUnlocked || bSelected ? 255 : LockedOpacity);
	Links->drawSegment(View.Root->getPosition(), ToCocos(Layout().TurretPoint), Layout().LinkWidth / 2, ToFill(bSelected ? Layout().LinkOn : Layout().LinkOff));
}

void UpgradesScene::RefreshPanel()
{
	if ( IsTab(Focused) )
	{
		const viewmodel::UpgradeWeaponTab& Tab = Layout().Weapons[Focused - Tree.GetNodeCount()];
		Name->setString(Text(Tab.NameKey));
		Info->setString(std::string());
		Level->setString(std::string());
		Need->setString(std::string());
		Buy->SetText(Text("UPGRADE_BUY"));
		Buy->SetEnabled(false);
		return;
	}
	const UpgradeDef& Upgrade = *Upgrades[Focused];
	const int Cost = Shop().GetNextCost(Upgrade.Id);
	const bool bIsOpen = Shop().IsOpen(Upgrade.Id);
	Name->setString(Text(Upgrade.NameKey));
	Info->setString(Text(Upgrade.NameKey + "_INFO"));
	Level->setString(Text("UPGRADE_LEVEL") + " " + std::to_string(Shop().GetLevel(Upgrade.Id)) + " / " + std::to_string(Upgrade.MaxLevel));
	Need->setString(bIsOpen ? std::string() : GetRequirementText(Upgrade));
	Buy->SetText(Cost < 0 ? Text("UPGRADE_MAX") : (bIsOpen ? Text("UPGRADE_BUY") + "  " + std::to_string(Cost) : Text("UPGRADE_LOCKED")));
	Buy->SetEnabled(Shop().CanBuy(Upgrade.Id));
}

void UpgradesScene::SetFocus(const int InIndex)
{
	if ( InIndex < 0 || InIndex >= GetFocusCount() )
	{
		return;
	}
	Focused = InIndex;
	if ( IsTab(Focused) && Layout().Weapons[Focused - Tree.GetNodeCount()].Branch != SelectedWeapon )
	{
		SelectedWeapon = Layout().Weapons[Focused - Tree.GetNodeCount()].Branch;
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
	std::vector<model::Vec2f> Positions;
	std::vector<bool> Selectable;
	for ( int Index = 0; Index < GetFocusCount(); ++Index )
	{
		const cocos2d::Vec2 Position = GetFocusPosition(Index);
		Positions.emplace_back(Position.x, Position.y);
		Selectable.push_back(IsShown(Index));
	}
	SetFocus(model::NodeNavigator::FindNext(Positions, Selectable, Focused, InDirection));
}

void UpgradesScene::Confirm()
{
	if ( IsTab(Focused) )
	{
		for ( int Index = 0; Index < Tree.GetNodeCount(); ++Index )
		{
			if ( IsShown(Index) && IsWeaponBranch(Upgrades[Index]->Branch) && Upgrades[Index]->Step == 0 )
			{
				SetFocus(Index);
			}
		}
		return;
	}
	if ( Focused < 0 || Focused >= Tree.GetNodeCount() || !Shop().Buy(Upgrades[Focused]->Id) )
	{
		return;
	}
	GameContext::Get().GetManager<AudioManager>().Play(ids::sounds::BonusPickup);
	Refresh();
}

bool UpgradesScene::IsShown(const int InIndex) const
{
	if ( IsTab(InIndex) )
	{
		return true;
	}
	const std::string& Branch = Upgrades[InIndex]->Branch;
	return !Shop().IsHidden(Upgrades[InIndex]->Id) && (!IsWeaponBranch(Branch) || Branch == SelectedWeapon);
}

bool UpgradesScene::IsTab(const int InIndex) const
{
	return InIndex >= Tree.GetNodeCount() && InIndex < GetFocusCount();
}

bool UpgradesScene::IsWeaponBranch(const std::string& InBranch) const
{
	const auto& Weapons = Layout().Weapons;
	return std::any_of(Weapons.begin(), Weapons.end(), [&InBranch](const viewmodel::UpgradeWeaponTab& InTab) { return InTab.Branch == InBranch; });
}

bool UpgradesScene::IsWeaponUnlocked(const std::string& InBranch) const
{
	for ( const UpgradeDef& Upgrade : Shop().GetUpgrades() )
	{
		if ( Upgrade.Branch == InBranch && !Upgrade.UnlockWeapon.empty() && Shop().GetLevel(Upgrade.Id) == 0 )
		{
			return false;
		}
	}
	return true;
}

int UpgradesScene::GetFocusCount() const
{
	return Tree.GetNodeCount() + static_cast<int>(Tabs.size());
}

cocos2d::Vec2 UpgradesScene::GetFocusPosition(const int InIndex) const
{
	return IsTab(InIndex) ? Tabs[InIndex - Tree.GetNodeCount()].Root->getPosition() : Views[InIndex].Root->getPosition();
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
	const UpgradeDef* const Requirement = Shop().GetRequirement(InUpgrade.Id);
	if ( !Requirement )
	{
		return std::string();
	}
	const std::string RequiredLevel = InUpgrade.RequiresLevel > 1 ? " " + Text("UPGRADE_LEVEL") + " " + std::to_string(InUpgrade.RequiresLevel) : std::string();
	return Text("UPGRADE_NEEDS") + ": " + Text(Requirement->NameKey) + RequiredLevel;
}

} // namespace tanks::view::cocos
