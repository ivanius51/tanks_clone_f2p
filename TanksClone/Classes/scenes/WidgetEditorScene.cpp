#include "scenes/WidgetEditorScene.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <set>

#include "core/BuildInfo.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "model/EditorLibrary.h"
#include "model/ResourceId.h"
#include "scenes/MenuScene.h"
#include "scenes/SceneUtils.h"
#include "services/UiStyle.h"
#include "ui/LayoutWidget.h"
#include "ui/MenuBuilder.h"
#include "services/StringsManager.h"
#include "ui/Widgets.h"

namespace game::view::cocos
{

namespace
{
	constexpr float Margin = 16.0f;
	constexpr float RowHeight = 30.0f;
	constexpr float TreeRowHeight = 16.0f;
	constexpr float ButtonHeight = 28.0f;
	constexpr float StepButtonWidth = 34.0f;
	constexpr float SidePanelWidth = 290.0f;
	constexpr float TreeWidth = 240.0f;
	constexpr float TopBarHeight = 52.0f;
	constexpr float BottomBarHeight = 28.0f;
	constexpr float PanelRowHeight = 26.0f;
	constexpr int StyleRow = -2;
	constexpr float PickerWidth = 600.0f;
	constexpr float PickerRowHeight = 20.0f;
	constexpr int PickerRows = 24;
	constexpr float PickerTitleHeight = 28.0f;
	constexpr float TreeIndent = 12.0f;
	constexpr int MaxZoom = 4;
	constexpr int MaxNesting = 8;
	constexpr int WheelRows = 3;

	std::string Short(const float InValue)
	{
		return cocos2d::StringUtils::format("%g", InValue);
	}

	std::string NameOfFile(const config_refs::GroupEntry& InEntry)
	{
		return InEntry.Id.substr(ids::config_groups::Widgets.size());
	}

	std::string NameOfPart(const viewmodel::WidgetPart InPart)
	{
		switch ( InPart )
		{
			case viewmodel::WidgetPart::Widget: return "widget";
			case viewmodel::WidgetPart::Label: return "label";
			case viewmodel::WidgetPart::Icon: return "icon";
			case viewmodel::WidgetPart::Rect: return "rect";
			case viewmodel::WidgetPart::Ring: return "ring";
			case viewmodel::WidgetPart::Bar: return "bar";
			case viewmodel::WidgetPart::Spacer: return "spacer";
		}
		return "";
	}

	bool InMenuMode(const viewmodel::MenuEditSession* const InMenu)
	{
		return InMenu && (InMenu->HasSelection() || InMenu->IsStyleSelected());
	}

	cocos2d::Rect WorldBox(const cocos2d::Node* const InNode)
	{
		const cocos2d::Rect Local = InNode->getBoundingBox();
		const cocos2d::Node* const Parent = InNode->getParent();
		const cocos2d::Vec2 A = Parent->convertToWorldSpace(Local.origin);
		const cocos2d::Vec2 B = Parent->convertToWorldSpace(Local.origin + Local.size);
		return cocos2d::Rect(std::min(A.x, B.x), std::min(A.y, B.y), std::abs(B.x - A.x), std::abs(B.y - A.y));
	}

	cocos2d::Rect OwnBox(const cocos2d::Node* const InWidget)
	{
		const cocos2d::Size Size = InWidget->getContentSize();
		const cocos2d::Vec2 A = InWidget->convertToWorldSpace(cocos2d::Vec2(-Size.width / 2, -Size.height / 2));
		const cocos2d::Vec2 B = InWidget->convertToWorldSpace(cocos2d::Vec2(Size.width / 2, Size.height / 2));
		return cocos2d::Rect(std::min(A.x, B.x), std::min(A.y, B.y), std::abs(B.x - A.x), std::abs(B.y - A.y));
	}
}

cocos2d::Scene* WidgetEditorScene::CreateScene()
{
	return WidgetEditorScene::create();
}

bool WidgetEditorScene::init()
{
	if ( !Scene::init() )
	{
		return false;
	}
	addChild(widgets::CreateScreenBackground());
	Files = config_refs::ListGroupEntries(ids::config_groups::Widgets);
	widgets::AddBackKeyHandler(this, []() { SwitchScene(MenuScene::CreateScene()); });

	auto* const Touch = cocos2d::EventListenerTouchOneByOne::create();
	Touch->onTouchBegan = [this](cocos2d::Touch* const InTouch, cocos2d::Event*) { return PickAt(InTouch->getLocation()); };
	_eventDispatcher->addEventListenerWithSceneGraphPriority(Touch, this);
	auto* const Mouse = cocos2d::EventListenerMouse::create();
	Mouse->onMouseScroll = [this](cocos2d::Event* const InEvent)
	{
		const auto* const Scroll = static_cast<cocos2d::EventMouse*>(InEvent);
		if ( PickerNode )
		{
			const int Most = std::max(0, static_cast<int>(PickerEntries.size()) - PickerRows);
			PickerScroll = std::clamp(PickerScroll + (Scroll->getScrollY() > 0 ? -3 : 3), 0, Most);
			BuildPicker();
		}
		else if ( Scroll->getCursorX() < TreeWidth )
		{
			TreeScroll = std::max(0, TreeScroll + (Scroll->getScrollY() > 0 ? WheelRows : -WheelRows));
			Rebuild();
		}
	};
	_eventDispatcher->addEventListenerWithSceneGraphPriority(Mouse, this);

	for ( const config_refs::GroupEntry& Entry : config_refs::ListGroupEntries(ids::config_groups::Views) )
	{
		Views.push_back(viewmodel::ViewDef::FromConfig(config_refs::GetConfig(model::ResourceId(Entry.Id)), Entry.Id.substr(ids::config_groups::Views.size())));
	}
	Rebuild();
	return true;
}

void WidgetEditorScene::onExit()
{
	if ( IsAnyDirty() )
	{
		GameContext::Get<UiStyle>().ReloadWidgets();
	}
	Scene::onExit();
}

viewmodel::WidgetEditSession& WidgetEditorScene::GetSession(const std::string& InFile)
{
	TUniquePtr<viewmodel::WidgetEditSession>& Found = Sessions[InFile];
	if ( !Found )
	{
		const viewmodel::WidgetDef* const Def = GameContext::Get<UiStyle>().FindWidget(InFile);
		Found = MakeUnique<viewmodel::WidgetEditSession>(Def ? *Def : viewmodel::WidgetDef());
	}
	return *Found;
}

viewmodel::WidgetEditSession& WidgetEditorScene::GetActive()
{
	return GetSession(ActiveFile);
}

bool WidgetEditorScene::IsAnyDirty() const
{
	const auto bIsDirty = [](const auto& InEntry) { return InEntry.second->IsDirty(); };
	return std::any_of(Sessions.begin(), Sessions.end(), bIsDirty) || std::any_of(MenuSessions.begin(), MenuSessions.end(), bIsDirty);
}

bool WidgetEditorScene::ShowView(const std::string& InId)
{
	const auto Found = std::find_if(Views.begin(), Views.end(), [&InId](const viewmodel::ViewDef& InView) { return InView.Id == InId; });
	if ( Found == Views.end() )
	{
		return false;
	}
	OpenView(static_cast<int>(Found - Views.begin()));
	return ViewIndex >= 0;
}

void WidgetEditorScene::OpenView(const int InIndex)
{
	const int Count = static_cast<int>(Views.size());
	ViewIndex = ((InIndex % Count) + Count) % Count;
	const viewmodel::ViewDef& View = Views[static_cast<size_t>(ViewIndex)];
	const viewmodel::MenuDef* const Menu = View.Menu.empty() ? nullptr : GameContext::Get<UiStyle>().FindMenu(View.Menu);
	const std::string Root = Menu ? Menu->Root : View.Root;
	const auto Found = std::find_if(Files.begin(), Files.end(), [&Root](const config_refs::GroupEntry& InEntry) { return NameOfFile(InEntry) == Root; });
	if ( Found == Files.end() )
	{
		ViewIndex = -1;
		Rebuild();
		SetMessage("View root widget not found: " + Root);
		return;
	}
	RootIndex = static_cast<int>(Found - Files.begin());
	ActiveMenu = nullptr;
	if ( Menu )
	{
		TUniquePtr<viewmodel::MenuEditSession>& MenuSession = MenuSessions[Menu->Id];
		if ( !MenuSession )
		{
			MenuSession = MakeUnique<viewmodel::MenuEditSession>(*Menu);
		}
		ActiveMenu = MenuSession.Get();
		ActiveMenu->Select(viewmodel::MenuEditSession::NoneSelected);
	}
	ActiveFile = Root;
	ActiveChain.clear();
	TreeScroll = 0;
	GetActive().Select(viewmodel::WidgetEditSession::WidgetSelected);
	Rebuild();
}

void WidgetEditorScene::ShowList()
{
	ListNode = cocos2d::Node::create();
	addChild(ListNode, 2);
	constexpr float ItemWidth = 420.0f;
	constexpr float Step = ButtonHeight + 8.0f;
	float Y = DesignHeight - 2 * Margin - ButtonHeight - 20.0f;
	auto* const Title = widgets::CreateLabel("Views: pick a window to edit", 20.0f);
	Title->setPosition(DesignWidth / 2, Y + 24.0f);
	ListNode->addChild(Title);
	for ( size_t Index = 0; Index < Views.size(); ++Index )
	{
		auto* const Button = widgets::Button::create(Views[Index].Title + "  (" + Views[Index].Root + ")", cocos2d::Size(ItemWidth, ButtonHeight), [this, Index]() { OpenView(static_cast<int>(Index)); });
		Button->setPosition(DesignWidth / 2, Y);
		ListNode->addChild(Button);
		Y -= Step;
	}
	auto* const Back = widgets::Button::create("Back", cocos2d::Size(80.0f, ButtonHeight), []() { SwitchScene(MenuScene::CreateScene()); });
	Back->setPosition(Margin + 40.0f, DesignHeight - Margin - ButtonHeight / 2);
	ListNode->addChild(Back);
	Message = widgets::CreateLabel(Views.empty() ? "No views in configs/ui/views" : "", 14.0f);
	Message->setPosition(DesignWidth / 2, Margin);
	ListNode->addChild(Message);
}

void WidgetEditorScene::Rebuild()
{
	UiStyle& Style = GameContext::Get<UiStyle>();
	for ( const auto& Entry : Sessions )
	{
		Style.ReplaceWidget(Entry.second->GetDef());
	}
	for ( cocos2d::Node* const Old : { static_cast<cocos2d::Node*>(Picture), static_cast<cocos2d::Node*>(Viewport), SidePanel, TopBar, Tree, static_cast<cocos2d::Node*>(Selection), ListNode } )
	{
		if ( Old )
		{
			Old->removeFromParent();
		}
	}
	MenuButtons.clear();
	Picture = nullptr;
	Viewport = nullptr;
	Selection = nullptr;
	SidePanel = nullptr;
	TopBar = nullptr;
	Tree = nullptr;
	ListNode = nullptr;
	Message = nullptr;
	Rows.clear();
	if ( ViewIndex < 0 )
	{
		ActiveMenu = nullptr;
		ShowList();
		return;
	}
	AppendRows(NameOfFile(Files[static_cast<size_t>(RootIndex)]), 0, {});
	const viewmodel::MenuDef* const Menu = ActiveMenu ? &ActiveMenu->GetDef() : nullptr;
	if ( Menu )
	{
		Rows.push_back({ "", viewmodel::WidgetEditSession::WidgetSelected, 1, "menu: button size", {}, StyleRow });
	}
	for ( size_t Index = 0; Menu && Index < Menu->Items.size(); ++Index )
	{
		const viewmodel::MenuItemDef& Item = Menu->Items[Index];
		std::string Label = "menu: " + Item.Id + (Item.bDebug ? " [debug]" : "");
		Label += Item.When.empty() ? "" : " (if " + Item.When + ")";
		Rows.push_back({ "", viewmodel::WidgetEditSession::WidgetSelected, 1, Label, {}, static_cast<int>(Index) });
	}
	BuildPicture();
	BuildTree();
	BuildSidePanel();
	BuildTopBar();
	DrawSelection();
}

void WidgetEditorScene::AppendRows(const std::string& InFile, const int InDepth, const std::vector<std::string>& InChain)
{
	const viewmodel::WidgetDef& Def = GetSession(InFile).GetDef();
	if ( InChain.empty() )
	{
		Rows.push_back({ InFile, viewmodel::WidgetEditSession::WidgetSelected, InDepth, InFile, InChain });
	}
	for ( size_t Index = 0; Index < Def.Children.size(); ++Index )
	{
		const viewmodel::WidgetChildDef& Child = Def.Children[Index];
		std::string Label = (Child.Id.empty() ? std::string("-") : Child.Id) + " [" + NameOfPart(Child.Part);
		Label += Child.Part == viewmodel::WidgetPart::Widget ? ": " + Child.Widget + "]" : "]";
		Rows.push_back({ InFile, static_cast<int>(Index), InDepth + 1, Label, InChain });
		if ( Child.Part == viewmodel::WidgetPart::Widget && static_cast<int>(InChain.size()) < MaxNesting
			&& GameContext::Get<UiStyle>().FindWidget(Child.Widget) )
		{
			std::vector<std::string> Deeper = InChain;
			Deeper.push_back(Child.Id);
			AppendRows(Child.Widget, InDepth + 2, Deeper);
		}
	}
}

void WidgetEditorScene::BuildPicture()
{
	Picture = LayoutWidget::Create(NameOfFile(Files[static_cast<size_t>(RootIndex)]));
	if ( !Picture )
	{
		return;
	}
	Picture->setScale(static_cast<float>(Zoom));
	const cocos2d::Rect Area = GetViewportArea();
	Picture->setPosition(Area.getMidX(), Area.getMidY());
	Viewport = cocos2d::ClippingRectangleNode::create(Area);
	addChild(Viewport, 1);
	Viewport->addChild(Picture, 1);
	Selection = cocos2d::DrawNode::create();
	Viewport->addChild(Selection, 3);
	ApplySamples();
}

cocos2d::Rect WidgetEditorScene::GetViewportArea() const
{
	return cocos2d::Rect(TreeWidth, BottomBarHeight, DesignWidth - TreeWidth - SidePanelWidth, DesignHeight - TopBarHeight - BottomBarHeight);
}

void WidgetEditorScene::ApplyMenu(const viewmodel::MenuDef& InMenu)
{
	LayoutWidget* Container = Picture;
	size_t Start = 0;
	while ( !InMenu.Container.empty() && Container && Start <= InMenu.Container.size() )
	{
		const size_t End = InMenu.Container.find('/', Start);
		Container = Container->FindWidget(InMenu.Container.substr(Start, End == std::string::npos ? std::string::npos : End - Start));
		Start = End == std::string::npos ? InMenu.Container.size() + 1 : End + 1;
	}
	for ( const viewmodel::MenuTextDef& Current : InMenu.Texts )
	{
		Picture->SetText(Current.Child, MenuText(Current));
	}
	if ( !Container )
	{
		return;
	}
	const cocos2d::Size Size = widgets::GetMenuButtonSize(InMenu, *Picture);
	for ( const MenuEntry& Entry : ResolveMenuEntries(InMenu, MenuBindings(), true) )
	{
		std::string Caption = Entry.Caption;
		if ( Entry.Item->bDebug )
		{
			Caption += "  [debug]";
		}
		if ( !Entry.Item->When.empty() )
		{
			Caption += "  [if " + Entry.Item->When + "]";
		}
		const int Index = static_cast<int>(Entry.Item - InMenu.Items.data());
		auto* const Button = widgets::Button::create(Caption, Size, [this, Index]() { ActiveMenu->Select(Index); Rebuild(); });
		Button->SetControl(widgets::Button::ControlFromKind(Entry.Item->Kind), Entry.bChecked);
		MenuButtons.resize(InMenu.Items.size(), nullptr);
		MenuButtons[static_cast<size_t>(Index)] = Button;
		Container->AppendNode(Button);
	}
}

void WidgetEditorScene::ApplySamples()
{
	const auto Resolve = [this](const std::string& InPath, const bool bInLastIsPart)
	{
		std::vector<std::string> Segments;
		size_t Start = 0;
		while ( !InPath.empty() && Start <= InPath.size() )
		{
			const size_t End = InPath.find('/', Start);
			Segments.push_back(InPath.substr(Start, End == std::string::npos ? std::string::npos : End - Start));
			Start = End == std::string::npos ? InPath.size() + 1 : End + 1;
		}
		std::string Last;
		if ( bInLastIsPart && !Segments.empty() )
		{
			Last = Segments.back();
			Segments.pop_back();
		}
		return std::make_pair(FindOwner(Segments), Last);
	};
	const viewmodel::ViewDef& View = Views[static_cast<size_t>(ViewIndex)];
	if ( ActiveMenu )
	{
		ApplyMenu(ActiveMenu->GetDef());
	}
	for ( const auto& [Path, Text] : View.Texts )
	{
		const auto [Owner, Part] = Resolve(Path, true);
		if ( Owner )
		{
			Owner->SetText(Part, Text);
		}
	}
	for ( const viewmodel::ViewItemDef& Item : View.Items )
	{
		LayoutWidget* const Container = Resolve(Item.Path, false).first;
		for ( const std::string& Text : Item.Texts )
		{
			LayoutWidget* const Sample = Container ? LayoutWidget::Create(Item.Widget) : nullptr;
			if ( Sample )
			{
				Sample->SetText("text", Text);
				Container->AppendNode(Sample);
			}
		}
	}
}

void WidgetEditorScene::BuildTree()
{
	Tree = cocos2d::Node::create();
	addChild(Tree, 2);
	auto* const Drawing = cocos2d::DrawNode::create();
	Tree->addChild(Drawing);
	const float Top = DesignHeight - 2 * Margin - ButtonHeight;
	Drawing->drawSolidRect(cocos2d::Vec2(0, BottomBarHeight), cocos2d::Vec2(TreeWidth, DesignHeight - TopBarHeight), cocos2d::Color4F(0.06f, 0.07f, 0.1f, 1.0f));
	Drawing->drawRect(cocos2d::Vec2(0, BottomBarHeight), cocos2d::Vec2(TreeWidth, DesignHeight - TopBarHeight), cocos2d::Color4F(0.3f, 0.34f, 0.45f, 1.0f));
	const viewmodel::WidgetEditSession& Active = GetActive();
	const int Visible = static_cast<int>((Top - BottomBarHeight) / TreeRowHeight);
	TreeScroll = std::clamp(TreeScroll, 0, std::max(0, static_cast<int>(Rows.size()) - Visible));
	for ( int Slot = 0; Slot < Visible && Slot + TreeScroll < static_cast<int>(Rows.size()); ++Slot )
	{
		const TreeRow& Row = Rows[static_cast<size_t>(Slot + TreeScroll)];
		const float Y = Top - (Slot + 0.5f) * TreeRowHeight;
		const bool bMenuSelected = InMenuMode(ActiveMenu);
		const bool bSelected = Row.File.empty() ? bMenuSelected && (Row.MenuItem == StyleRow ? ActiveMenu->IsStyleSelected() : ActiveMenu->HasSelection() && Row.MenuItem == ActiveMenu->GetSelected())
			: !bMenuSelected && Row.File == ActiveFile && Row.Chain == ActiveChain && Row.Index == Active.GetSelected();
		if ( bSelected )
		{
			Drawing->drawSolidRect(cocos2d::Vec2(Margin / 2, Y - TreeRowHeight / 2), cocos2d::Vec2(TreeWidth, Y + TreeRowHeight / 2), cocos2d::Color4F(0.35f, 0.3f, 0.1f, 1.0f));
		}
		auto* const Text = widgets::CreateLabel(Row.Label, 11.0f);
		Text->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
		Text->setPosition(Margin + Row.Depth * TreeIndent, Y);
		Tree->addChild(Text);
	}
}

void WidgetEditorScene::BuildTopBar()
{
	TopBar = cocos2d::Node::create();
	addChild(TopBar, 0);
	auto* const Bars = cocos2d::DrawNode::create();
	TopBar->addChild(Bars);
	const cocos2d::Color4F BarFill(0.06f, 0.07f, 0.1f, 1.0f);
	const cocos2d::Color4F BarLine(0.3f, 0.34f, 0.45f, 1.0f);
	Bars->drawSolidRect(cocos2d::Vec2(0, DesignHeight - TopBarHeight), cocos2d::Vec2(DesignWidth, DesignHeight), BarFill);
	Bars->drawSolidRect(cocos2d::Vec2(0, 0), cocos2d::Vec2(DesignWidth, BottomBarHeight), BarFill);
	Bars->drawSolidRect(cocos2d::Vec2(DesignWidth - SidePanelWidth, BottomBarHeight), cocos2d::Vec2(DesignWidth, DesignHeight - TopBarHeight), BarFill);
	const cocos2d::Rect Area = GetViewportArea();
	Bars->drawRect(Area.origin, Area.origin + Area.size, BarLine);
	Bars->drawRect(cocos2d::Vec2(DesignWidth - SidePanelWidth, BottomBarHeight), cocos2d::Vec2(DesignWidth, DesignHeight - TopBarHeight), BarLine);
	float X = Margin;
	const float Y = DesignHeight - Margin - ButtonHeight;
	const auto AddButton = [&](const std::string& InText, const float InWidth, const TFunction<void(), UiCallbackInlineSize>& InAction)
	{
		auto* const Button = widgets::Button::create(InText, cocos2d::Size(InWidth, ButtonHeight), InAction);
		Button->setPosition(X + InWidth / 2, Y + ButtonHeight / 2);
		TopBar->addChild(Button);
		X += InWidth + 6.0f;
		return Button;
	};
	AddButton("<", StepButtonWidth, [this]() { OpenView(ViewIndex - 1); });
	auto* const Title = widgets::CreateLabel(Views[static_cast<size_t>(ViewIndex)].Title + (IsAnyDirty() ? " *" : ""), 18.0f);
	Title->setPosition(X + 90.0f, Y + ButtonHeight / 2);
	TopBar->addChild(Title);
	X += 180.0f;
	AddButton(">", StepButtonWidth, [this]() { OpenView(ViewIndex + 1); });
	X += 10.0f;
	const bool bMenuMode = InMenuMode(ActiveMenu);
	AddButton("Undo", 62.0f, [this, bMenuMode]() { bMenuMode ? ActiveMenu->Undo() : GetActive().Undo(); Rebuild(); })->SetEnabled(bMenuMode ? ActiveMenu->CanUndo() : GetActive().CanUndo());
	AddButton("Redo", 62.0f, [this, bMenuMode]() { bMenuMode ? ActiveMenu->Redo() : GetActive().Redo(); Rebuild(); })->SetEnabled(bMenuMode ? ActiveMenu->CanRedo() : GetActive().CanRedo());
	AddButton("Zoom " + std::to_string(Zoom) + "x", 80.0f, [this]() { Zoom = Zoom % MaxZoom + 1; Rebuild(); });
	AddButton("Save", 64.0f, [this]() { Save(); });
	AddButton("Views", 64.0f, [this]() { ViewIndex = -1; Rebuild(); });
	Message = widgets::CreateLabel("", 14.0f);
	Message->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
	Message->setPosition(Margin, Margin / 2);
	TopBar->addChild(Message);
}

std::vector<std::string> WidgetEditorScene::ChoicesFor(const viewmodel::MenuField InField) const
{
	if ( InField == viewmodel::MenuField::Kind )
	{
		return { "choice", "toggle", "stepper" };
	}
	std::set<std::string> Found;
	const UiStyle& Style = GameContext::Get<UiStyle>();
	if ( InField == viewmodel::MenuField::TextKey )
	{
		const std::vector<std::string> Keys = GameContext::Get<StringsManager>().GetAllKeys();
		Found.insert(Keys.begin(), Keys.end());
	}
	const auto Collect = [&](const viewmodel::MenuDef& InMenu)
	{
		for ( const viewmodel::MenuItemDef& Item : InMenu.Items )
		{
			switch ( InField )
			{
			case viewmodel::MenuField::Value: Found.insert(Item.ValueId); break;
			case viewmodel::MenuField::Action: Found.insert(Item.Action); break;
			case viewmodel::MenuField::When:
			case viewmodel::MenuField::EnabledWhen: Found.insert(Item.When); Found.insert(Item.EnabledWhen); break;
			case viewmodel::MenuField::TextKey: Found.insert(Item.TextKey); break;
			case viewmodel::MenuField::Kind: break;
			}
		}
	};
	for ( const auto& Entry : Style.GetMenus() )
	{
		Collect(Entry.second);
	}
	if ( ActiveMenu )
	{
		Collect(ActiveMenu->GetDef());
	}
	Found.erase(std::string());
	return std::vector<std::string>(Found.begin(), Found.end());
}

void WidgetEditorScene::BuildMenuItemPanel()
{
	const float Left = DesignWidth - SidePanelWidth + 8.0f;
	float Y = DesignHeight - 2 * Margin - ButtonHeight - RowHeight;
	viewmodel::MenuEditSession& Menu = *ActiveMenu;
	const auto AddButton = [this](const std::string& InText, const float InX, const float InY, const float InWidth, const TFunction<void(), UiCallbackInlineSize>& InAction)
	{
		auto* const Button = widgets::Button::create(InText, cocos2d::Size(InWidth, ButtonHeight), InAction);
		Button->setPosition(InX + InWidth / 2, InY);
		SidePanel->addChild(Button);
		return Button;
	};
	const auto AddLabel = [this, Left](const std::string& InText, const float InY, const float InSize)
	{
		auto* const Label = widgets::CreateLabel(InText.size() > 22 ? InText.substr(0, 21) + "~" : InText, InSize);
		Label->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
		Label->setPosition(Left, InY);
		SidePanel->addChild(Label);
	};
	if ( Menu.IsStyleSelected() )
	{
		AddLabel("menu: button size", Y, 14.0f);
		Y -= RowHeight * 1.5f;
		const std::string& Style = Menu.GetDef().ButtonStyle;
		const bool bHasSpacing = Style == "main" || Style == "settings";
		for ( const viewmodel::MenuMetric Metric : { viewmodel::MenuMetric::ButtonWidth, viewmodel::MenuMetric::ButtonHeight, viewmodel::MenuMetric::ButtonSpacing } )
		{
			if ( Metric == viewmodel::MenuMetric::ButtonSpacing && !bHasSpacing )
			{
				continue;
			}
			const char* const Name = Metric == viewmodel::MenuMetric::ButtonWidth ? "width" : Metric == viewmodel::MenuMetric::ButtonHeight ? "height" : "spacing";
			const float Value = Menu.GetMetric(Metric);
			AddLabel(std::string(Name) + ": " + (Value > 0.0f ? Short(Value) : std::string("style")), Y, 13.0f);
			AddButton("-", Left + SidePanelWidth - Margin - 2 * StepButtonWidth - 6.0f, Y, StepButtonWidth, [this, Metric]() { ActiveMenu->NudgeMetric(Metric, -1); Rebuild(); });
			AddButton("+", Left + SidePanelWidth - Margin - StepButtonWidth, Y, StepButtonWidth, [this, Metric]() { ActiveMenu->NudgeMetric(Metric, 1); Rebuild(); });
			Y -= RowHeight;
		}
		return;
	}
	AddLabel("menu item: " + Menu.GetDef().Items[static_cast<size_t>(Menu.GetSelected())].Id, Y, 14.0f);
	Y -= RowHeight * 1.5f;
	const float RightEdge = Left + SidePanelWidth - Margin;
	for ( const viewmodel::MenuField Field : viewmodel::MenuEditSession::GetFields() )
	{
		const std::string Value = Menu.GetValue(Field);
		AddLabel(viewmodel::MenuEditSession::GetFieldName(Field) + ": " + (Value.empty() ? "-" : Value), Y, 13.0f);
		AddButton("<", RightEdge - 2 * StepButtonWidth - 6.0f, Y, StepButtonWidth, [this, Field]() { ActiveMenu->Cycle(Field, -1, ChoicesFor(Field)); Rebuild(); });
		AddButton(">", RightEdge - StepButtonWidth, Y, StepButtonWidth, [this, Field]() { ActiveMenu->Cycle(Field, 1, ChoicesFor(Field)); Rebuild(); });
		if ( Field != viewmodel::MenuField::Kind )
		{
			AddButton("...", RightEdge - 2 * StepButtonWidth - 6.0f - StepButtonWidth - 6.0f, Y, StepButtonWidth, [this, Field]()
			{
				OpenPicker(viewmodel::MenuEditSession::GetFieldName(Field), ActiveMenu->GetValue(Field), ChoicesFor(Field), Field == viewmodel::MenuField::TextKey,
					[this, Field](const std::string& InValue) { ActiveMenu->SetValue(Field, InValue); });
			});
		}
		Y -= RowHeight;
		if ( Field == viewmodel::MenuField::TextKey && !Value.empty() )
		{
			AddLabel("= " + GameContext::Get<StringsManager>().GetString(Value), Y + RowHeight * 0.25f, 12.0f);
			Y -= RowHeight * 0.5f;
		}
	}
	const viewmodel::MenuItemDef& Item = Menu.GetDef().Items[static_cast<size_t>(Menu.GetSelected())];
	if ( Item.Kind == "choice" )
	{
		AddLabel(Item.OptionsSource.empty() ? "options: " + std::to_string(Item.Options.size()) : "options from: " + Item.OptionsSource, Y, 13.0f);
		AddButton("Add", RightEdge - 2 * 52.0f - 6.0f, Y, 52.0f, [this]()
			{
				OpenPicker("option to add", std::string(), AllStringKeys(), true, [this](const std::string& InValue) { ActiveMenu->AddOption(InValue); });
			});
		AddButton("Del", RightEdge - 52.0f, Y, 52.0f, [this]() { ActiveMenu->RemoveLastOption(); Rebuild(); })->SetEnabled(!Item.Options.empty());
		Y -= RowHeight;
		const StringsManager& Strings = GameContext::Get<StringsManager>();
		for ( const std::string& Option : Item.Options )
		{
			AddLabel("  " + Option + " = " + Strings.GetString(Option), Y + RowHeight * 0.25f, 12.0f);
			Y -= RowHeight * 0.5f;
		}
		Y -= RowHeight * 0.5f;
	}
	if ( Item.Kind == "stepper" )
	{
		for ( const viewmodel::MenuNumber Number : { viewmodel::MenuNumber::Min, viewmodel::MenuNumber::Max, viewmodel::MenuNumber::Step } )
		{
			const char* const Name = Number == viewmodel::MenuNumber::Min ? "min" : Number == viewmodel::MenuNumber::Max ? "max" : "step";
			AddLabel(std::string(Name) + ": " + Short(Menu.GetNumber(Number)), Y, 13.0f);
			AddButton("-", RightEdge - 2 * StepButtonWidth - 6.0f, Y, StepButtonWidth, [this, Number]() { ActiveMenu->NudgeNumber(Number, -1); Rebuild(); });
			AddButton("+", RightEdge - StepButtonWidth, Y, StepButtonWidth, [this, Number]() { ActiveMenu->NudgeNumber(Number, 1); Rebuild(); });
			Y -= RowHeight;
		}
	}
	AddLabel(std::string("debug: ") + (Menu.IsDebug() ? "yes" : "no"), Y, 13.0f);
	AddButton("Toggle", RightEdge - 70.0f, Y, 70.0f, [this]() { ActiveMenu->ToggleDebug(); Rebuild(); });
	Y -= RowHeight * 1.5f;
	constexpr float ActionWidth = 60.0f;
	AddButton("Up", Left, Y, ActionWidth, [this]() { ActiveMenu->MoveSelected(-1); Rebuild(); })->SetEnabled(Menu.CanMove(-1));
	AddButton("Down", Left + ActionWidth + 6.0f, Y, ActionWidth, [this]() { ActiveMenu->MoveSelected(1); Rebuild(); })->SetEnabled(Menu.CanMove(1));
	AddButton("Copy", Left + 2 * (ActionWidth + 6.0f), Y, ActionWidth, [this]() { ActiveMenu->DuplicateSelected(); Rebuild(); });
	AddButton("Delete", Left + 3 * (ActionWidth + 6.0f), Y, ActionWidth, [this]() { ActiveMenu->RemoveSelected(); Rebuild(); });
}

std::vector<std::string> WidgetEditorScene::AllStringKeys() const
{
	return GameContext::Get<StringsManager>().GetAllKeys();
}

void WidgetEditorScene::OpenPicker(const std::string& InTitle, const std::string& InCurrent, const std::vector<std::string>& InChoices, const bool bInPreview, const TFunction<void(const std::string&), UiCallbackInlineSize>& InApply)
{
	PickerTitle = InTitle;
	PickerCurrent = InCurrent;
	PickerApply = InApply;
	PickerEntries.clear();
	PickerEntries.push_back({ std::string(), "(none)" });
	const StringsManager& Strings = GameContext::Get<StringsManager>();
	for ( const std::string& Choice : InChoices )
	{
		PickerEntries.push_back({ Choice, bInPreview ? Strings.GetString(Choice) : std::string() });
	}
	const auto Found = std::find_if(PickerEntries.begin(), PickerEntries.end(), [&InCurrent](const auto& InEntry) { return InEntry.first == InCurrent; });
	PickerScroll = std::max(0, static_cast<int>(Found - PickerEntries.begin()) - PickerRows / 2);
	BuildPicker();
}

void WidgetEditorScene::ClosePicker()
{
	if ( PickerNode )
	{
		PickerNode->removeFromParent();
		PickerNode = nullptr;
	}
}

void WidgetEditorScene::BuildPicker()
{
	ClosePicker();
	PickerNode = cocos2d::Node::create();
	addChild(PickerNode, 10);
	auto* const Drawing = cocos2d::DrawNode::create();
	PickerNode->addChild(Drawing);
	const float Left = (DesignWidth - PickerWidth) / 2;
	const float Top = DesignHeight - Margin * 2;
	const float Height = PickerTitleHeight + PickerRows * PickerRowHeight;
	Drawing->drawSolidRect(cocos2d::Vec2(0, 0), cocos2d::Vec2(DesignWidth, DesignHeight), cocos2d::Color4F(0.0f, 0.0f, 0.0f, 0.6f));
	Drawing->drawSolidRect(cocos2d::Vec2(Left, Top - Height), cocos2d::Vec2(Left + PickerWidth, Top), cocos2d::Color4F(0.08f, 0.09f, 0.13f, 1.0f));
	Drawing->drawRect(cocos2d::Vec2(Left, Top - Height), cocos2d::Vec2(Left + PickerWidth, Top), cocos2d::Color4F(0.4f, 0.45f, 0.6f, 1.0f));
	const auto AddText = [this](const std::string& InText, const float InX, const float InY, const float InSize)
	{
		auto* const Label = widgets::CreateLabel(InText, InSize);
		Label->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
		Label->setPosition(InX, InY);
		PickerNode->addChild(Label);
	};
	AddText("Pick " + PickerTitle + " (wheel scrolls, click outside cancels)", Left + 8.0f, Top - PickerTitleHeight / 2, 13.0f);
	const std::string& Current = PickerCurrent;
	for ( int Slot = 0; Slot < PickerRows && Slot + PickerScroll < static_cast<int>(PickerEntries.size()); ++Slot )
	{
		const auto& Entry = PickerEntries[static_cast<size_t>(Slot + PickerScroll)];
		const float Y = Top - PickerTitleHeight - (Slot + 0.5f) * PickerRowHeight;
		if ( Entry.first == Current )
		{
			Drawing->drawSolidRect(cocos2d::Vec2(Left + 2.0f, Y - PickerRowHeight / 2), cocos2d::Vec2(Left + PickerWidth - 2.0f, Y + PickerRowHeight / 2), cocos2d::Color4F(0.35f, 0.3f, 0.1f, 1.0f));
		}
		AddText(Entry.first.empty() ? Entry.second : Entry.first, Left + 8.0f, Y, 12.0f);
		if ( !Entry.first.empty() && !Entry.second.empty() )
		{
			AddText(Entry.second.size() > 42 ? Entry.second.substr(0, 41) + "~" : Entry.second, Left + 250.0f, Y, 12.0f);
		}
	}
	auto* const Touch = cocos2d::EventListenerTouchOneByOne::create();
	Touch->setSwallowTouches(true);
	Touch->onTouchBegan = [this](cocos2d::Touch* const InTouch, cocos2d::Event*) { return PickInPicker(InTouch->getLocation()); };
	_eventDispatcher->addEventListenerWithSceneGraphPriority(Touch, PickerNode);
}

bool WidgetEditorScene::PickInPicker(const cocos2d::Vec2& InPoint)
{
	const float Left = (DesignWidth - PickerWidth) / 2;
	const float Top = DesignHeight - Margin * 2;
	const float RowsTop = Top - PickerTitleHeight;
	const int Slot = static_cast<int>((RowsTop - InPoint.y) / PickerRowHeight) + PickerScroll;
	const bool bInside = InPoint.x >= Left && InPoint.x <= Left + PickerWidth && InPoint.y < RowsTop && InPoint.y > RowsTop - PickerRows * PickerRowHeight;
	if ( bInside && Slot >= 0 && Slot < static_cast<int>(PickerEntries.size()) )
	{
		const std::string Picked = PickerEntries[static_cast<size_t>(Slot)].first;
		const TFunction<void(const std::string&), UiCallbackInlineSize> Apply = PickerApply;
		ClosePicker();
		Apply(Picked);
		Rebuild();
		return true;
	}
	else if ( bInside )
	{
		return true;
	}
	ClosePicker();
	Rebuild();
	return true;
}

void WidgetEditorScene::BuildSidePanel()
{
	SidePanel = cocos2d::Node::create();
	addChild(SidePanel, 2);
	if ( InMenuMode(ActiveMenu) )
	{
		BuildMenuItemPanel();
		return;
	}
	const float Left = DesignWidth - SidePanelWidth + 8.0f;
	float Y = DesignHeight - 2 * Margin - ButtonHeight - RowHeight;
	viewmodel::WidgetEditSession& Active = GetActive();
	const auto AddButton = [this](const std::string& InText, const float InX, const float InY, const float InWidth, const TFunction<void(), UiCallbackInlineSize>& InAction)
	{
		auto* const Button = widgets::Button::create(InText, cocos2d::Size(InWidth, ButtonHeight), InAction);
		Button->setPosition(InX + InWidth / 2, InY);
		SidePanel->addChild(Button);
	};
	auto* const Name = widgets::CreateLabel(ActiveFile + ": " + Active.GetSelectedName(), 14.0f);
	Name->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
	Name->setPosition(Left, Y);
	SidePanel->addChild(Name);
	Y -= RowHeight * 1.5f;
	if ( Active.HasTextKey() )
	{
		const std::string Key = Active.GetTextKey();
		auto* const KeyLabel = widgets::CreateLabel("text key: " + (Key.empty() ? std::string("-") : Key.size() > 20 ? Key.substr(0, 19) + "~" : Key), 13.0f);
		KeyLabel->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
		KeyLabel->setPosition(Left, Y);
		SidePanel->addChild(KeyLabel);
		AddButton("...", Left + SidePanelWidth - Margin - 2 * StepButtonWidth - 6.0f, Y, StepButtonWidth, [this, Key]()
			{
				OpenPicker("text key", Key, AllStringKeys(), true, [this](const std::string& InValue) { GetActive().SetTextKey(InValue); });
			});
		AddButton("x", Left + SidePanelWidth - Margin - StepButtonWidth, Y, StepButtonWidth, [this]() { GetActive().SetTextKey(std::string()); Rebuild(); });
		Y -= RowHeight * 0.6f;
		if ( !Key.empty() )
		{
			auto* const Preview = widgets::CreateLabel("= " + GameContext::Get<StringsManager>().GetString(Key), 12.0f);
			Preview->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
			Preview->setPosition(Left, Y);
			SidePanel->addChild(Preview);
		}
		Y -= RowHeight;
	}
	for ( const viewmodel::EditProperty Property : viewmodel::WidgetEditSession::GetProperties() )
	{
		if ( !Active.IsApplicable(Property) )
		{
			continue;
		}
		auto* const Label = widgets::CreateLabel(viewmodel::WidgetEditSession::GetPropertyName(Property) + ": " + Short(Active.GetValue(Property)), 14.0f);
		Label->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
		Label->setPosition(Left, Y);
		SidePanel->addChild(Label);
		AddButton("-", Left + SidePanelWidth - Margin - 2 * StepButtonWidth - 6.0f, Y, StepButtonWidth, [this, Property]() { GetActive().Nudge(Property, -1); Rebuild(); });
		AddButton("+", Left + SidePanelWidth - Margin - StepButtonWidth, Y, StepButtonWidth, [this, Property]() { GetActive().Nudge(Property, 1); Rebuild(); });
		Y -= PanelRowHeight;
	}
	for ( const viewmodel::EditChoice Choice : viewmodel::WidgetEditSession::GetChoices() )
	{
		if ( !Active.IsChoiceApplicable(Choice) )
		{
			continue;
		}
		auto* const Label = widgets::CreateLabel(viewmodel::WidgetEditSession::GetChoiceName(Choice) + ": " + Active.GetChoiceValue(Choice), 14.0f);
		Label->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
		Label->setPosition(Left, Y);
		SidePanel->addChild(Label);
		AddButton("<", Left + SidePanelWidth - Margin - 2 * StepButtonWidth - 6.0f, Y, StepButtonWidth, [this, Choice]() { GetActive().CycleChoice(Choice, -1); Rebuild(); });
		AddButton(">", Left + SidePanelWidth - Margin - StepButtonWidth, Y, StepButtonWidth, [this, Choice]() { GetActive().CycleChoice(Choice, 1); Rebuild(); });
		Y -= PanelRowHeight;
	}
}

LayoutWidget* WidgetEditorScene::FindOwner(const std::vector<std::string>& InChain) const
{
	LayoutWidget* Owner = Picture;
	for ( const std::string& Id : InChain )
	{
		Owner = Owner ? Owner->FindWidget(Id) : nullptr;
	}
	return Owner;
}

void WidgetEditorScene::DrawSelection()
{
	if ( !Selection )
	{
		return;
	}
	if ( ActiveMenu && ActiveMenu->IsStyleSelected() )
	{
		for ( const cocos2d::Node* const Current : MenuButtons )
		{
			if ( Current )
			{
				const cocos2d::Rect Box = OwnBox(Current);
				Selection->drawRect(Box.origin, Box.origin + Box.size, cocos2d::Color4F(1.0f, 0.8f, 0.2f, 1.0f));
			}
		}
		return;
	}
	if ( ActiveMenu && ActiveMenu->HasSelection() )
	{
		const size_t Index = static_cast<size_t>(ActiveMenu->GetSelected());
		if ( Index < MenuButtons.size() && MenuButtons[Index] )
		{
			const cocos2d::Rect Box = OwnBox(MenuButtons[Index]);
			Selection->drawRect(Box.origin, Box.origin + Box.size, cocos2d::Color4F(1.0f, 0.8f, 0.2f, 1.0f));
		}
		return;
	}
	const LayoutWidget* const Owner = FindOwner(ActiveChain);
	if ( !Owner )
	{
		return;
	}
	const viewmodel::WidgetEditSession& Active = GetActive();
	cocos2d::Rect Box = OwnBox(Owner);
	if ( Active.GetSelected() != viewmodel::WidgetEditSession::WidgetSelected )
	{
		const viewmodel::WidgetChildDef& Child = Active.GetDef().Children[static_cast<size_t>(Active.GetSelected())];
		const cocos2d::Node* const Part = Child.Id.empty() ? nullptr : Owner->FindPart(Child.Id);
		if ( Part )
		{
			Box = Child.Part == viewmodel::WidgetPart::Widget ? OwnBox(Part) : WorldBox(Part);
		}
	}
	Selection->drawRect(Box.origin, Box.origin + Box.size, cocos2d::Color4F(1.0f, 0.8f, 0.2f, 1.0f));
}

bool WidgetEditorScene::PickIn(LayoutWidget* const InOwner, const std::string& InFile, const std::vector<std::string>& InChain, const cocos2d::Vec2& InPoint, TreeRow& OutRow) const
{
	const auto Found = Sessions.find(InFile);
	if ( Found == Sessions.end() )
	{
		return false;
	}
	const std::vector<viewmodel::WidgetChildDef>& Children = Found->second->GetDef().Children;
	for ( int Index = static_cast<int>(Children.size()) - 1; Index >= 0; --Index )
	{
		const viewmodel::WidgetChildDef& Child = Children[static_cast<size_t>(Index)];
		if ( Child.Id.empty() )
		{
			continue;
		}
		if ( Child.Part == viewmodel::WidgetPart::Widget )
		{
			std::vector<std::string> Deeper = InChain;
			Deeper.push_back(Child.Id);
			LayoutWidget* const Sub = InOwner->FindWidget(Child.Id);
			if ( Sub && PickIn(Sub, Child.Widget, Deeper, InPoint, OutRow) )
			{
				return true;
			}
		}
		const cocos2d::Node* const Part = InOwner->FindPart(Child.Id);
		if ( Part && Part->isVisible() && WorldBox(Part).containsPoint(InPoint) )
		{
			OutRow = { InFile, Index, 0, "", InChain };
			return true;
		}
	}
	return false;
}

void WidgetEditorScene::SelectRow(const TreeRow& InRow)
{
	if ( InRow.File.empty() )
	{
		if ( ActiveMenu && InRow.MenuItem == StyleRow )
		{
			ActiveMenu->SelectStyle();
			Rebuild();
		}
		else if ( ActiveMenu && InRow.MenuItem >= 0 )
		{
			ActiveMenu->Select(InRow.MenuItem);
			Rebuild();
		}
		return;
	}
	if ( ActiveMenu )
	{
		ActiveMenu->Select(viewmodel::MenuEditSession::NoneSelected);
	}
	ActiveFile = InRow.File;
	ActiveChain = InRow.Chain;
	GetActive().Select(InRow.Index);
	Rebuild();
}

bool WidgetEditorScene::PickAt(const cocos2d::Vec2& InPoint)
{
	if ( InPoint.x < TreeWidth )
	{
		const float Top = DesignHeight - 2 * Margin - ButtonHeight;
		const int Slot = static_cast<int>((Top - InPoint.y) / TreeRowHeight) + TreeScroll;
		if ( InPoint.y < Top && Slot >= 0 && Slot < static_cast<int>(Rows.size()) )
		{
			SelectRow(Rows[static_cast<size_t>(Slot)]);
			return true;
		}
		return false;
	}
	if ( !Picture || !GetViewportArea().containsPoint(InPoint) )
	{
		return false;
	}
	TreeRow Hit;
	const std::string RootFile = NameOfFile(Files[static_cast<size_t>(RootIndex)]);
	if ( PickIn(Picture, RootFile, {}, InPoint, Hit) )
	{
		SelectRow(Hit);
		return true;
	}
	if ( OwnBox(Picture).containsPoint(InPoint) )
	{
		SelectRow({ RootFile, viewmodel::WidgetEditSession::WidgetSelected, 0, "", {} });
		return true;
	}
	return false;
}

void WidgetEditorScene::SetMessage(const std::string& InText)
{
	if ( Message )
	{
		Message->setString(InText);
	}
}

void WidgetEditorScene::Save()
{
	const std::string Root = GameContext::Get<UiStyle>().GetEditor().DevResourcesRoot;
	if ( !bHasDebugTools || Root.empty() || !std::filesystem::is_directory(Root) )
	{
		SetMessage("No source folder (dev_resources_root): not saved");
		return;
	}
	std::string Saved;
	for ( const auto& Entry : Sessions )
	{
		if ( !Entry.second->IsDirty() )
		{
			continue;
		}
		const auto File = std::find_if(Files.begin(), Files.end(), [&](const config_refs::GroupEntry& InFile) { return NameOfFile(InFile) == Entry.first; });
		if ( File == Files.end() )
		{
			continue;
		}
		std::ofstream Out(model::JoinSourcePath(Root, File->Path), std::ios::binary | std::ios::trunc);
		Out << Entry.second->ToText();
		if ( !Out.good() )
		{
			SetMessage("Could not write " + File->Path);
			return;
		}
		Entry.second->MarkSaved();
		Saved += (Saved.empty() ? "" : ", ") + Entry.first;
	}
	for ( const config_refs::GroupEntry& Entry : config_refs::ListGroupEntries(ids::config_groups::Menus) )
	{
		const std::string Name = Entry.Id.substr(ids::config_groups::Menus.size());
		const auto Found = MenuSessions.find(Name);
		if ( Found == MenuSessions.end() || !Found->second->IsDirty() )
		{
			continue;
		}
		std::ofstream Out(model::JoinSourcePath(Root, Entry.Path), std::ios::binary | std::ios::trunc);
		Out << Found->second->ToText();
		if ( !Out.good() )
		{
			SetMessage("Could not write " + Entry.Path);
			return;
		}
		Found->second->MarkSaved();
		GameContext::Get<UiStyle>().ReplaceMenu(Found->second->GetDef());
		Saved += (Saved.empty() ? "" : ", ") + Name;
	}
	Rebuild();
	SetMessage(Saved.empty() ? "Nothing to save" : "Saved " + Saved);
}

} // namespace game::view::cocos
