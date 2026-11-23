#include "scenes/WidgetEditorScene.h"

#include <filesystem>
#include <fstream>

#include "core/BuildInfo.h"
#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "model/EditorLibrary.h"
#include "model/ResourceId.h"
#include "scenes/MenuScene.h"
#include "scenes/SceneUtils.h"
#include "services/UiStyle.h"
#include "ui/CocosConvert.h"
#include "ui/LayoutWidget.h"
#include "ui/Widgets.h"

namespace tanks::view::cocos
{

namespace
{
	constexpr float Margin = 16.0f;
	constexpr float RowHeight = 30.0f;
	constexpr float ButtonHeight = 28.0f;
	constexpr float StepButtonWidth = 34.0f;
	constexpr float SidePanelWidth = 330.0f;
	constexpr int MaxZoom = 4;

	std::string Short(const float InValue)
	{
		return cocos2d::StringUtils::format("%g", InValue);
	}

	std::string NameOfFile(const config_refs::GroupEntry& InEntry)
	{
		return InEntry.Id.substr(ids::config_groups::Widgets.size());
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
	if ( !Files.empty() )
	{
		OpenWidget(0);
	}
	return true;
}

void WidgetEditorScene::OpenWidget(const int InIndex)
{
	const int Count = static_cast<int>(Files.size());
	FileIndex = ((InIndex % Count) + Count) % Count;
	const config_refs::GroupEntry& Entry = Files[static_cast<size_t>(FileIndex)];
	std::vector<std::string> Missing;
	Session = std::make_unique<viewmodel::WidgetEditSession>(
		viewmodel::WidgetDef::FromConfig(config_refs::GetConfig(model::ResourceId(Entry.Id)), NameOfFile(Entry), Missing));
	Rebuild();
}

void WidgetEditorScene::Rebuild()
{
	for ( cocos2d::Node* const Old : { static_cast<cocos2d::Node*>(Preview), SidePanel, TopBar } )
	{
		if ( Old )
		{
			Old->removeFromParent();
		}
	}
	Preview = nullptr;
	Message = nullptr;
	BuildPreview();
	BuildSidePanel();
	BuildTopBar();
}

void WidgetEditorScene::BuildPreview()
{
	Preview = LayoutWidget::Create(Session->GetDef());
	if ( !Preview )
	{
		return;
	}
	Preview->setScale(static_cast<float>(Zoom));
	Preview->setPosition((DesignWidth - SidePanelWidth) / 2, DesignHeight / 2 - 20.0f);
	addChild(Preview, 1);
	Selection = cocos2d::DrawNode::create();
	Preview->addChild(Selection, 100000);
	DrawSelection();
}

void WidgetEditorScene::DrawSelection()
{
	Selection->clear();
	const int Selected = Session->GetSelected();
	cocos2d::Rect Box;
	if ( Selected == viewmodel::WidgetEditSession::WidgetSelected )
	{
		const cocos2d::Size Size = Preview->getContentSize();
		Box = cocos2d::Rect(-Size.width / 2, -Size.height / 2, Size.width, Size.height);
	}
	else
	{
		const cocos2d::Node* const Part = Preview->FindPart(Session->GetDef().Children[static_cast<size_t>(Selected)].Id);
		if ( !Part )
		{
			return;
		}
		Box = Part->getBoundingBox();
	}
	Selection->drawRect(Box.origin, Box.origin + Box.size, cocos2d::Color4F(1.0f, 0.8f, 0.2f, 1.0f));
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
	AddButton("<", StepButtonWidth, [this]() { OpenWidget(FileIndex - 1); });
	auto* const Title = widgets::CreateLabel(NameOfFile(Files[static_cast<size_t>(FileIndex)]) + (Session->IsDirty() ? " *" : ""), 18.0f);
	Title->setPosition(X + 120.0f, Y + ButtonHeight / 2);
	TopBar->addChild(Title);
	X += 240.0f;
	AddButton(">", StepButtonWidth, [this]() { OpenWidget(FileIndex + 1); });
	X += 20.0f;
	AddButton("Undo", 70.0f, [this]() { Session->Undo(); Rebuild(); })->SetEnabled(Session->CanUndo());
	AddButton("Redo", 70.0f, [this]() { Session->Redo(); Rebuild(); })->SetEnabled(Session->CanRedo());
	AddButton("Zoom " + std::to_string(Zoom) + "x", 90.0f, [this]() { Zoom = Zoom % MaxZoom + 1; Rebuild(); });
	AddButton("Save", 80.0f, [this]() { Save(); });
	AddButton("Back", 80.0f, []() { SwitchScene(MenuScene::CreateScene()); });
	Message = widgets::CreateLabel("", 16.0f);
	Message->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
	Message->setPosition(Margin, Margin);
	TopBar->addChild(Message);
}

void WidgetEditorScene::BuildSidePanel()
{
	SidePanel = cocos2d::Node::create();
	addChild(SidePanel, 2);
	const float Left = DesignWidth - SidePanelWidth;
	float Y = DesignHeight - 2 * Margin - ButtonHeight - RowHeight;
	const auto AddButton = [this](const std::string& InText, const float InX, const float InY, const float InWidth, const std::function<void()>& InAction)
	{
		auto* const Button = widgets::Button::create(InText, cocos2d::Size(InWidth, ButtonHeight), InAction);
		Button->setPosition(InX + InWidth / 2, InY);
		SidePanel->addChild(Button);
	};
	AddButton("<", Left, Y, StepButtonWidth, [this]() { Session->SelectNext(-1); Rebuild(); });
	auto* const Name = widgets::CreateLabel(Session->GetSelectedName(), 18.0f);
	Name->setPosition(Left + SidePanelWidth / 2 - 10.0f, Y);
	SidePanel->addChild(Name);
	AddButton(">", Left + SidePanelWidth - Margin - StepButtonWidth, Y, StepButtonWidth, [this]() { Session->SelectNext(1); Rebuild(); });
	Y -= RowHeight * 1.5f;
	for ( const viewmodel::EditProperty Property : viewmodel::WidgetEditSession::GetProperties() )
	{
		if ( !Session->IsApplicable(Property) )
		{
			continue;
		}
		auto* const Label = widgets::CreateLabel(viewmodel::WidgetEditSession::GetPropertyName(Property) + ": " + Short(Session->GetValue(Property)), 16.0f);
		Label->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
		Label->setPosition(Left, Y);
		SidePanel->addChild(Label);
		AddButton("-", Left + SidePanelWidth - Margin - 2 * StepButtonWidth - 6.0f, Y, StepButtonWidth, [this, Property]() { Session->Nudge(Property, -1); Rebuild(); });
		AddButton("+", Left + SidePanelWidth - Margin - StepButtonWidth, Y, StepButtonWidth, [this, Property]() { Session->Nudge(Property, 1); Rebuild(); });
		Y -= RowHeight;
	}
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
	const std::string Path = Files[static_cast<size_t>(FileIndex)].Path;
	if ( !bHasDebugTools || Root.empty() || !std::filesystem::is_directory(Root) )
	{
		SetMessage("No source folder (dev_resources_root): not saved");
		return;
	}
	std::ofstream File(model::JoinSourcePath(Root, Path), std::ios::binary | std::ios::trunc);
	File << Session->ToText();
	if ( !File.good() )
	{
		SetMessage("Could not write " + Path);
		return;
	}
	Session->MarkSaved();
	Rebuild();
	SetMessage("Saved " + Path);
}

} // namespace tanks::view::cocos
