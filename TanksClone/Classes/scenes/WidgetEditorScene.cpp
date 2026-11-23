#include "scenes/WidgetEditorScene.h"

#include <algorithm>
#include <filesystem>
#include <fstream>

#include "core/BuildInfo.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "model/EditorLibrary.h"
#include "scenes/MenuScene.h"
#include "scenes/SceneUtils.h"
#include "services/UiStyle.h"
#include "ui/LayoutWidget.h"
#include "ui/Widgets.h"

namespace tanks::view::cocos
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
		if ( Scroll->getCursorX() < TreeWidth )
		{
			TreeScroll = std::max(0, TreeScroll + (Scroll->getScrollY() > 0 ? WheelRows : -WheelRows));
			Rebuild();
		}
	};
	_eventDispatcher->addEventListenerWithSceneGraphPriority(Mouse, this);

	if ( !Files.empty() )
	{
		const auto Menu = std::find_if(Files.begin(), Files.end(), [](const config_refs::GroupEntry& InEntry) { return NameOfFile(InEntry) == "overlay"; });
		OpenRoot(Menu == Files.end() ? 0 : static_cast<int>(Menu - Files.begin()));
	}
	return true;
}

void WidgetEditorScene::onExit()
{
	if ( IsAnyDirty() )
	{
		GameContext::Get().GetManager<UiStyle>().ReloadWidgets();
	}
	Scene::onExit();
}

viewmodel::WidgetEditSession& WidgetEditorScene::GetSession(const std::string& InFile)
{
	std::unique_ptr<viewmodel::WidgetEditSession>& Found = Sessions[InFile];
	if ( !Found )
	{
		const viewmodel::WidgetDef* const Def = GameContext::Get().GetManager<UiStyle>().FindWidget(InFile);
		Found = std::make_unique<viewmodel::WidgetEditSession>(Def ? *Def : viewmodel::WidgetDef());
	}
	return *Found;
}

viewmodel::WidgetEditSession& WidgetEditorScene::GetActive()
{
	return GetSession(ActiveFile);
}

bool WidgetEditorScene::IsAnyDirty() const
{
	return std::any_of(Sessions.begin(), Sessions.end(), [](const auto& InEntry) { return InEntry.second->IsDirty(); });
}

void WidgetEditorScene::OpenRoot(const int InIndex)
{
	const int Count = static_cast<int>(Files.size());
	RootIndex = ((InIndex % Count) + Count) % Count;
	ActiveFile = NameOfFile(Files[static_cast<size_t>(RootIndex)]);
	ActiveChain.clear();
	TreeScroll = 0;
	GetActive().Select(viewmodel::WidgetEditSession::WidgetSelected);
	Rebuild();
}

void WidgetEditorScene::Rebuild()
{
	UiStyle& Style = GameContext::Get().GetManager<UiStyle>();
	for ( const auto& Entry : Sessions )
	{
		Style.ReplaceWidget(Entry.second->GetDef());
	}
	for ( cocos2d::Node* const Old : { static_cast<cocos2d::Node*>(Picture), SidePanel, TopBar, Tree, static_cast<cocos2d::Node*>(Selection) } )
	{
		if ( Old )
		{
			Old->removeFromParent();
		}
	}
	Picture = nullptr;
	Selection = nullptr;
	Message = nullptr;
	Rows.clear();
	AppendRows(NameOfFile(Files[static_cast<size_t>(RootIndex)]), 0, {});
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
			&& GameContext::Get().GetManager<UiStyle>().FindWidget(Child.Widget) )
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
	Picture->setPosition(TreeWidth + (DesignWidth - TreeWidth - SidePanelWidth) / 2, DesignHeight / 2 - 20.0f);
	addChild(Picture, 1);
	Selection = cocos2d::DrawNode::create();
	addChild(Selection, 3);
}

void WidgetEditorScene::BuildTree()
{
	Tree = cocos2d::Node::create();
	addChild(Tree, 2);
	auto* const Drawing = cocos2d::DrawNode::create();
	Tree->addChild(Drawing);
	const float Top = DesignHeight - 2 * Margin - ButtonHeight;
	const viewmodel::WidgetEditSession& Active = GetActive();
	const int Visible = static_cast<int>((Top - Margin) / TreeRowHeight);
	TreeScroll = std::clamp(TreeScroll, 0, std::max(0, static_cast<int>(Rows.size()) - Visible));
	for ( int Slot = 0; Slot < Visible && Slot + TreeScroll < static_cast<int>(Rows.size()); ++Slot )
	{
		const TreeRow& Row = Rows[static_cast<size_t>(Slot + TreeScroll)];
		const float Y = Top - (Slot + 0.5f) * TreeRowHeight;
		const bool bSelected = Row.File == ActiveFile && Row.Chain == ActiveChain && Row.Index == Active.GetSelected();
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
	addChild(TopBar, 2);
	float X = Margin;
	const float Y = DesignHeight - Margin - ButtonHeight;
	const auto AddButton = [&](const std::string& InText, const float InWidth, const std::function<void()>& InAction)
	{
		auto* const Button = widgets::Button::create(InText, cocos2d::Size(InWidth, ButtonHeight), InAction);
		Button->setPosition(X + InWidth / 2, Y + ButtonHeight / 2);
		TopBar->addChild(Button);
		X += InWidth + 6.0f;
		return Button;
	};
	AddButton("<", StepButtonWidth, [this]() { OpenRoot(RootIndex - 1); });
	auto* const Title = widgets::CreateLabel(NameOfFile(Files[static_cast<size_t>(RootIndex)]) + (IsAnyDirty() ? " *" : ""), 18.0f);
	Title->setPosition(X + 90.0f, Y + ButtonHeight / 2);
	TopBar->addChild(Title);
	X += 180.0f;
	AddButton(">", StepButtonWidth, [this]() { OpenRoot(RootIndex + 1); });
	X += 10.0f;
	AddButton("Undo", 62.0f, [this]() { GetActive().Undo(); Rebuild(); })->SetEnabled(GetActive().CanUndo());
	AddButton("Redo", 62.0f, [this]() { GetActive().Redo(); Rebuild(); })->SetEnabled(GetActive().CanRedo());
	AddButton("Zoom " + std::to_string(Zoom) + "x", 80.0f, [this]() { Zoom = Zoom % MaxZoom + 1; Rebuild(); });
	AddButton("Save", 64.0f, [this]() { Save(); });
	AddButton("Back", 64.0f, []() { SwitchScene(MenuScene::CreateScene()); });
	Message = widgets::CreateLabel("", 14.0f);
	Message->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
	Message->setPosition(Margin, Margin / 2);
	TopBar->addChild(Message);
}

void WidgetEditorScene::BuildSidePanel()
{
	SidePanel = cocos2d::Node::create();
	addChild(SidePanel, 2);
	const float Left = DesignWidth - SidePanelWidth;
	float Y = DesignHeight - 2 * Margin - ButtonHeight - RowHeight;
	viewmodel::WidgetEditSession& Active = GetActive();
	const auto AddButton = [this](const std::string& InText, const float InX, const float InY, const float InWidth, const std::function<void()>& InAction)
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
	for ( const viewmodel::EditProperty Property : viewmodel::WidgetEditSession::GetProperties() )
	{
		if ( !Active.IsApplicable(Property) )
		{
			continue;
		}
		auto* const Label = widgets::CreateLabel(viewmodel::WidgetEditSession::GetPropertyName(Property) + ": " + Short(Active.GetValue(Property)), 16.0f);
		Label->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
		Label->setPosition(Left, Y);
		SidePanel->addChild(Label);
		AddButton("-", Left + SidePanelWidth - Margin - 2 * StepButtonWidth - 6.0f, Y, StepButtonWidth, [this, Property]() { GetActive().Nudge(Property, -1); Rebuild(); });
		AddButton("+", Left + SidePanelWidth - Margin - StepButtonWidth, Y, StepButtonWidth, [this, Property]() { GetActive().Nudge(Property, 1); Rebuild(); });
		Y -= RowHeight;
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
	const LayoutWidget* const Owner = FindOwner(ActiveChain);
	if ( !Owner )
	{
		return;
	}
	const viewmodel::WidgetEditSession& Active = GetActive();
	cocos2d::Rect Box = OwnBox(Owner);
	if ( Active.GetSelected() != viewmodel::WidgetEditSession::WidgetSelected )
	{
		const std::string& ChildId = Active.GetDef().Children[static_cast<size_t>(Active.GetSelected())].Id;
		const cocos2d::Node* const Part = ChildId.empty() ? nullptr : Owner->FindPart(ChildId);
		if ( Part )
		{
			Box = WorldBox(Part);
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
	if ( !Picture )
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
	const std::string Root = GameContext::Get().GetManager<UiStyle>().GetEditor().DevResourcesRoot;
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
	Rebuild();
	SetMessage(Saved.empty() ? "Nothing to save" : "Saved " + Saved);
}

} // namespace tanks::view::cocos
