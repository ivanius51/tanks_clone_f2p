#include "ui/Widgets.h"

#include <cmath>
#include <unordered_set>

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "services/AudioManager.h"
#include "services/InputManager.h"
#include "services/ProfileManager.h"
#include "services/ResourceManager.h"
#include "services/StringsManager.h"
#include "services/UiStyle.h"
#include "ui/ActionListener.h"
#include "ui/CocosConvert.h"
#include "ui/FontAtlasHold.h"
#include "ui/ScreenFit.h"
#include "ui/LayoutWidget.h"
#include "ui/MenuBuilder.h"

namespace tanks::view::cocos
{
namespace widgets
{

namespace
{
	constexpr float GridStep = TileSize;

	const viewmodel::WidgetStyle& Style()
	{
		return GameContext::Get().GetManager<UiStyle>().GetWidgets();
	}

	model::ResourceId FontId(const bool bInDecorative)
	{
		const std::string& Override = GameContext::Get().GetManager<StringsManager>().GetFontId(bInDecorative);
		if ( Override.empty() )
		{
			return bInDecorative ? ids::fonts::Decorative : ids::fonts::Plain;
		}
		return model::ResourceId(Override);
	}

	std::string FontPath(const bool bInDecorative)
	{
		return GameContext::Get().GetManager<ResourceManager>().ResolvePath(FontId(bInDecorative), model::ResourceKind::Font);
	}
	
void DrawBorderedRect(cocos2d::DrawNode* const InNode, const cocos2d::Size& InSize, const cocos2d::Color4F& InFill, const cocos2d::Color4F& InBorder)
	{
		const cocos2d::Vec2 Low(-InSize.width / 2, -InSize.height / 2);
		const cocos2d::Vec2 High(InSize.width / 2, InSize.height / 2);
		InNode->drawSolidRect(Low, High, InBorder);
		InNode->drawSolidRect(Low + cocos2d::Vec2(Style().BorderWidth, Style().BorderWidth), High - cocos2d::Vec2(Style().BorderWidth, Style().BorderWidth), InFill);
	}
}

namespace colors
{
cocos2d::Color3B TextNormal()
{
	return ToCocos3B(Style().Colors.TextNormal);
}

cocos2d::Color3B TextDim()
{
	return ToCocos3B(Style().Colors.TextDim);
}

cocos2d::Color3B Accent()
{
	return ToCocos3B(Style().Colors.Accent);
}

cocos2d::Color3B Good()
{
	return ToCocos3B(Style().Colors.Good);
}

cocos2d::Color3B Bad()
{
	return ToCocos3B(Style().Colors.Bad);
}

cocos2d::Color4F Panel()
{
	return cocos2d::Color4F(ToCocos(Style().Colors.Panel));
}

cocos2d::Color4F PanelBorder()
{
	return cocos2d::Color4F(ToCocos(Style().Colors.PanelBorder));
}

cocos2d::Color4F ButtonColor()
{
	return cocos2d::Color4F(ToCocos(Style().Colors.ButtonColor));
}

cocos2d::Color4F ButtonPressed()
{
	return cocos2d::Color4F(ToCocos(Style().Colors.ButtonPressed));
}

cocos2d::Color4F ButtonDisabled()
{
	return cocos2d::Color4F(ToCocos(Style().Colors.ButtonDisabled));
}

cocos2d::Color4F ButtonBorder()
{
	return cocos2d::Color4F(ToCocos(Style().Colors.ButtonBorder));
}

cocos2d::Color4F Selection()
{
	return cocos2d::Color4F(ToCocos(Style().Colors.Selection));
}

cocos2d::Color4B Outline()
{
	return ToCocos(Style().Colors.Outline);
}
}

void AddOutline(cocos2d::Label* const InLabel)
{
	InLabel->enableOutline(colors::Outline(), Style().LabelOutline);
}

float GetUiScale()
{
	return Style().TextScales.Get(GameContext::Get().GetManager<ProfileManager>().Get().CurrentTextSize);
}

cocos2d::Size ScaleUi(const cocos2d::Size& InSize)
{
	return InSize * GetUiScale();
}

cocos2d::Label* CreateLabel(const std::string& InText, const float InFontSize, const cocos2d::Color3B& InColor, const bool bInDecorative)
{
	const float FontSize = InFontSize * GetUiScale();
	ResourceManager& Resources = GameContext::Get().GetManager<ResourceManager>();
	const std::string_view FontName = FontId(bInDecorative).GetName();
	const std::string Path = FontPath(bInDecorative);
	cocos2d::Label* CurrentLabel = Path.empty() ? nullptr : cocos2d::Label::createWithTTF(InText, Path, bInDecorative ? FontSize * Style().DecorativeFontScale : FontSize);
	if ( !CurrentLabel )
	{
		Resources.ReportMissing(ResourceManager::FontKind, FontName, Path.empty() ? "no path" : Path);
		CurrentLabel = cocos2d::Label::createWithSystemFont(InText, Style().SystemFont, FontSize);
	}
	else
	{
		Resources.ReportOk(ResourceManager::FontKind, FontName);
		auto* const ScaleListener = cocos2d::EventListenerCustom::create(ContentScaleChangedEvent, [CurrentLabel](cocos2d::EventCustom*)
		{
			CurrentLabel->setTTFConfig(CurrentLabel->getTTFConfig());
		});
		CurrentLabel->getEventDispatcher()->addEventListenerWithSceneGraphPriority(ScaleListener, CurrentLabel);
		auto* const HoldListener = cocos2d::EventListenerCustom::create(ContentScaleChangingEvent, [CurrentLabel](cocos2d::EventCustom* const InEvent)
		{
			static_cast<FontAtlasHold*>(InEvent->getUserData())->Add(CurrentLabel->getFontAtlas());
		});
		CurrentLabel->getEventDispatcher()->addEventListenerWithSceneGraphPriority(HoldListener, CurrentLabel);
	}
	CurrentLabel->setColor(InColor);
	return CurrentLabel;
}

cocos2d::Node* CreateScreenBackground()
{
	auto CurrentRoot = cocos2d::LayerColor::create(ToCocos(Style().Colors.ScreenBackground), DesignWidth, DesignHeight);
	auto CurrentGrid = cocos2d::DrawNode::create();
	CurrentRoot->addChild(CurrentGrid);
	CoverScreen(CurrentRoot);
	OnScreenLayoutChanged(CurrentGrid, [CurrentGrid](const model::ScreenLayout& InLayout)
	{
		// The lines stay on the design grid, however far the visible area reaches past it.
		const model::RectF Visible = InLayout.GetVisibleRect();
		const cocos2d::Vec2 FirstLine(std::fmod(-Visible.Origin.X, GridStep), std::fmod(-Visible.Origin.Y, GridStep));
		const cocos2d::Color4F Line(ToCocos(Style().Colors.GridLine));
		CurrentGrid->clear();
		for ( float X = FirstLine.x; X <= Visible.Size.X; X += GridStep )
		{
			CurrentGrid->drawLine(cocos2d::Vec2(X, 0), cocos2d::Vec2(X, Visible.Size.Y), Line);
		}
		for ( float Y = FirstLine.y; Y <= Visible.Size.Y; Y += GridStep )
		{
			CurrentGrid->drawLine(cocos2d::Vec2(0, Y), cocos2d::Vec2(Visible.Size.X, Y), Line);
		}
	});
	return CurrentRoot;
}

cocos2d::Sprite* CreateFrameSprite(const std::string& InFrame)
{
	cocos2d::SpriteFrame* const CurrentFrame = GameContext::Get().GetManager<ResourceManager>().FindFrame(InFrame);
	return CurrentFrame ? cocos2d::Sprite::createWithSpriteFrame(CurrentFrame) : nullptr;
}

void AddBackKeyHandler(cocos2d::Node* const InOwner, const std::function<void()>& InOnBack, const bool bInOwnerIsOverlay)
{
	AddActionHandler(InOwner, [InOnBack, bInOwnerIsOverlay](const model::InputAction InAction)
	{
		if ( InAction == model::InputAction::UiBack && (bInOwnerIsOverlay || !Overlay::IsShown()) )
		{
			InOnBack();
		}
	});
}

// ---------------------------------------------------------------- Button

Button* Button::create(const std::string& InText, const cocos2d::Size& InSize, const std::function<void()>& InCallback)
{
	Button* NewButton = new (std::nothrow) Button();
	if ( NewButton && NewButton->init(InText, InSize, InCallback) )
	{
		NewButton->autorelease();
		return NewButton;
	}
	CC_SAFE_DELETE(NewButton);
	return nullptr;
}

namespace
{
	std::unordered_set<const cocos2d::Node*>& LiveButtons()
	{
		static std::unordered_set<const cocos2d::Node*> Buttons;
		return Buttons;
	}
}

Button* Button::FromNode(const Node* const InNode)
{
	return LiveButtons().count(InNode) ? static_cast<Button*>(const_cast<Node*>(InNode)) : nullptr;
}

Button::~Button()
{
	LiveButtons().erase(this);
}

bool Button::init(const std::string& InText, const cocos2d::Size& InSize, const std::function<void()>& InCallback)
{
	LiveButtons().insert(this);
	if ( !Node::init() )
	{
		return false;
	}

	Callback = InCallback;
	setContentSize(InSize);

	Background = cocos2d::DrawNode::create();
	addChild(Background);

	Content = LayoutWidget::Create(ids::widgets::Button);
	addChild(Content, 1);
	Content->SetFixedSize(InSize);
	Content->SetText("text", InText);

	auto CurrentListener = cocos2d::EventListenerTouchOneByOne::create();
	CurrentListener->setSwallowTouches(true);
	CurrentListener->onTouchBegan = [this](cocos2d::Touch* const InTouch, cocos2d::Event*)
	{
		if ( !bEnabled || !IsShown() || !IsTouchInside(InTouch) )
		{
			return false;
		}
		bPressed = true;
		Redraw();
		return true;
	};
	CurrentListener->onTouchEnded = [this](cocos2d::Touch* const InTouch, cocos2d::Event*)
	{
		const bool bClick = bPressed && IsTouchInside(InTouch);
		bPressed = false;
		Redraw();
		if ( bClick )
		{
			Activate();
		}
	};
	CurrentListener->onTouchCancelled = [this](cocos2d::Touch*, cocos2d::Event*)
	{
		bPressed = false;
		Redraw();
	};
	_eventDispatcher->addEventListenerWithSceneGraphPriority(CurrentListener, this);

	Redraw();
	return true;
}

void Button::SetText(const std::string& InText)
{
	Content->SetText("text", InText);
}

void Button::SetSize(const cocos2d::Size& InSize)
{
	setContentSize(InSize);
	Content->SetFixedSize(InSize);
	Redraw();
}

std::string Button::GetText() const
{
	return Content->GetText("text");
}

void Button::SetSelected(const bool bInSelected)
{
	bSelected = bInSelected;
	Redraw();
}

void Button::SetEnabled(const bool bInEnabled)
{
	bEnabled = bInEnabled;
	Redraw();
}

bool Button::IsEnabled() const
{
	return bEnabled;
}

void Button::Activate()
{
	if ( bEnabled && Callback )
	{
		GameContext::Get().GetManager<AudioManager>().Play(ids::sounds::UiClick);
		// The callback may replace the scene and delete this button.
		auto CurrentCallback = Callback;
		CurrentCallback();
	}
}

void Button::Redraw()
{
	const cocos2d::Size CurrentSize = getContentSize();
	const cocos2d::Color4F& Fill = !bEnabled ? colors::ButtonDisabled() : (bPressed ? colors::ButtonPressed() : colors::ButtonColor());

	Background->clear();
	DrawBorderedRect(Background, CurrentSize, Fill, bSelected && bEnabled ? colors::Selection() : colors::ButtonBorder());
	Content->SetTextColor("text", bEnabled ? (bSelected ? colors::Accent() : colors::TextNormal()) : colors::TextDim());
}

bool Button::IsTouchInside(cocos2d::Touch* const InTouch) const
{
	const cocos2d::Vec2 Local = convertToNodeSpace(InTouch->getLocation());
	const cocos2d::Size CurrentSize = getContentSize();
	return cocos2d::Rect(-CurrentSize.width / 2, -CurrentSize.height / 2, CurrentSize.width, CurrentSize.height).containsPoint(Local);
}

bool Button::IsShown() const
{
	for ( const Node* CurrentVisualNode = this; CurrentVisualNode; CurrentVisualNode = CurrentVisualNode->getParent() )
	{
		if ( !CurrentVisualNode->isVisible() )
		{
			return false;
		}
	}
	return true;
}

void Button::SetAdjust(const std::function<void(int)>& InAdjust)
{
	AdjustCallback = InAdjust;
}

void Button::Adjust(const int InDirection)
{
	if ( AdjustCallback && IsEnabled() )
	{
		AdjustCallback(InDirection);
	}
}

// ------------------------------------------------------------ ButtonList

bool ButtonList::init()
{
	if ( !Node::init() )
	{
		return false;
	}

	AddActionHandler(this, [this](const model::InputAction InAction)
	{
		if ( Overlay::IsShown() && !bBelongsToOverlay )
		{
			return;
		}
		switch ( InAction )
		{
			case model::InputAction::UiUp:
				MoveSelection(-1);
				break;
			case model::InputAction::UiDown:
				MoveSelection(1);
				break;
			case model::InputAction::UiLeft:
			case model::InputAction::UiRight:
				if ( Selected >= 0 && Selected < static_cast<int>(Buttons.size()) )
				{
					Buttons[Selected]->Adjust(InAction == model::InputAction::UiLeft ? -1 : 1);
				}
				break;
			case model::InputAction::UiConfirm:
				if ( Selected >= 0 && Selected < static_cast<int>(Buttons.size()) )
				{
					Buttons[Selected]->Activate();
				}
				break;
			default:
				break;
		}
	});
	return true;
}

void ButtonList::AddButton(Button* const InButton)
{
	Buttons.push_back(InButton);
	if ( Buttons.size() == 1 )
	{
		Select(0);
	}
}

void ButtonList::LayoutVertically(const cocos2d::Vec2& InCenter, const float InSpacing)
{
	if ( Buttons.empty() )
	{
		return;
	}

	const float CurrentStep = Buttons[0]->getContentSize().height + InSpacing;
	float Y = InCenter.y + CurrentStep * (Buttons.size() - 1) / 2;
	for ( auto CurrentButton : Buttons )
	{
		CurrentButton->setPosition(InCenter.x, Y);
		Y -= CurrentStep;
	}
}

void ButtonList::Select(const int InIndex)
{
	Selected = InIndex;
	for ( int Index = 0; Index < static_cast<int>(Buttons.size()); ++Index )
	{
		Buttons[Index]->SetSelected(Index == Selected);
	}
}

int ButtonList::GetSelected() const
{
	return Selected;
}

void ButtonList::SetBelongsToOverlay(const bool bInBelongs)
{
	bBelongsToOverlay = bInBelongs;
}

void ButtonList::Clear()
{
	Buttons.clear();
	Selected = 0;
}

void ButtonList::MoveSelection(const int InStep)
{
	const int CurrentCount = static_cast<int>(Buttons.size());
	for ( int Index = 1; Index <= CurrentCount; ++Index )
	{
		const int Candidate = ((Selected + InStep * Index) % CurrentCount + CurrentCount) % CurrentCount;
		if ( Buttons[Candidate]->IsEnabled() )
		{
			if ( Candidate != Selected )
			{
				GameContext::Get().GetManager<AudioManager>().Play(ids::sounds::UiMove);
			}
			Select(Candidate);
			return;
		}
	}
}

// --------------------------------------------------------------- Overlay

namespace
{
	int SShownOverlays = 0;
}

cocos2d::Node* CreateControlHints(const bool bInForOverlay)
{
	LayoutWidget* const Hints = LayoutWidget::Create(ids::widgets::ControlHints);
	if ( !Hints )
	{
		return cocos2d::Node::create();
	}
	Hints->schedule([Hints, bInForOverlay](float)
	{
		const InputManager& Input = GameContext::Get().GetManager<InputManager>();
		const StringsManager& Strings = GameContext::Get().GetManager<StringsManager>();
		const auto Show = [&](const char* const InChild, const model::InputAction InAction, const char* const InTextId)
		{
			LayoutWidget* const Hint = Hints->FindWidget(InChild);
			Hint->FindWidget("key")->SetText("text", Input.GetButtonLabel(model::InputContext::Menu, InAction));
			Hint->SetText("text", Strings.GetString(InTextId));
		};
		Show("select", model::InputAction::UiConfirm, "HINT_SELECT");
		Show("back", model::InputAction::UiBack, "HINT_BACK");
		Show("change", model::InputAction::UiRight, "HINT_CHANGE");
		Hints->FindWidget("change")->FindWidget("key")->SetText("text", Input.GetButtonLabel(model::InputContext::Menu, model::InputAction::UiLeft));
		Hints->FindWidget("change")->FindWidget("key_right")->SetText("text", Input.GetButtonLabel(model::InputContext::Menu, model::InputAction::UiRight));
		Hints->setPosition(DesignWidth / 2, Style().HintsMargin + Hints->getContentSize().height / 2);
		Hints->setVisible(bInForOverlay || !Overlay::IsShown());
	}, "control_hints");
	return Hints;
}

bool Overlay::IsShown()
{
	return SShownOverlays > 0;
}

void Overlay::onEnter()
{
	LayerColor::onEnter();
	++SShownOverlays;
}

void Overlay::onExit()
{
	--SShownOverlays;
	LayerColor::onExit();
}

Overlay* Overlay::create(const std::string& InTitle, const std::string& InMessage, const std::vector<ButtonSpec>& InButtons)
{
	Overlay* NewOverlay = new (std::nothrow) Overlay();
	if ( NewOverlay && NewOverlay->init(InTitle, InMessage, InButtons) )
	{
		NewOverlay->autorelease();
		return NewOverlay;
	}
	CC_SAFE_DELETE(NewOverlay);
	return nullptr;
}

Overlay* Overlay::CreateEmpty()
{
	Overlay* NewOverlay = new (std::nothrow) Overlay();
	if ( NewOverlay && NewOverlay->InitLayer() )
	{
		NewOverlay->autorelease();
		return NewOverlay;
	}
	CC_SAFE_DELETE(NewOverlay);
	return nullptr;
}

cocos2d::Size GetMenuButtonSize(const std::string_view InStyle, const LayoutWidget& InPanel)
{
	const viewmodel::MenuLayout& Menu = GameContext::Get().GetManager<UiStyle>().GetMenu();
	if ( InStyle == "main" )
	{
		return ScaleUi(cocos2d::Size(Menu.Main.ButtonWidth, Menu.Main.ButtonHeight));
	}
	if ( InStyle == "settings" )
	{
		return ScaleUi(cocos2d::Size(Menu.Settings.ButtonWidth, Menu.Settings.ButtonHeight));
	}
	const viewmodel::WidgetDef& Def = InPanel.GetDef();
	const float Scale = GetUiScale();
	return cocos2d::Size((Def.Rules.FixedSize.X - 2 * Def.Rules.Padding.X) * Scale, Style().ButtonHeight * Scale);
}

Overlay* Overlay::CreateFromMenu(const std::string_view InMenuId, const MenuBindings& InBindings)
{
	const viewmodel::MenuDef* const Menu = GameContext::Get().GetManager<UiStyle>().FindMenu(InMenuId);
	if ( !Menu )
	{
		return nullptr;
	}
	const auto TextOf = [Menu](const char* const InChild)
	{
		const viewmodel::MenuTextDef* const Found = Menu->FindTextDef(InChild);
		return Found ? MenuText(*Found) : std::string();
	};
	std::vector<ButtonSpec> Buttons;
	for ( const MenuEntry& Entry : ResolveMenuEntries(*Menu, InBindings, false) )
	{
		Buttons.push_back({ Entry.Caption, Entry.Action });
	}
	return create(TextOf("title"), TextOf("message"), Buttons);
}

bool Overlay::InitLayer()
{
	if ( !LayerColor::initWithColor(cocos2d::Color4B(0, 0, 0, 0), DesignWidth, DesignHeight) )
	{
		return false;
	}
	// The overlay keeps design coordinates for its content; only the dimming reaches over the whole window.
	auto Dim = cocos2d::LayerColor::create(ToCocos(Style().Colors.Dim));
	addChild(Dim, -1);
	CoverScreen(Dim);

	// Block everything underneath.
	auto Blocker = cocos2d::EventListenerTouchOneByOne::create();
	Blocker->setSwallowTouches(true);
	Blocker->onTouchBegan = [](cocos2d::Touch*, cocos2d::Event*) { return true; };
	_eventDispatcher->addEventListenerWithSceneGraphPriority(Blocker, this);

	// Menu input while shown: nothing reaches the gameplay or the editor underneath.
	addChild(ActionListener::create([](const model::InputEvent&) {}, model::InputContext::Menu));
	return true;
}

bool Overlay::init(const std::string& InTitle, const std::string& InMessage, const std::vector<ButtonSpec>& InButtons)
{
	if ( !InitLayer() )
	{
		return false;
	}

	LayoutWidget* const Panel = LayoutWidget::Create(ids::widgets::Overlay);
	if ( !Panel )
	{
		return false;
	}
	Panel->SetText("title", InTitle);
	Panel->SetText("message", InMessage);
	Panel->SetPartVisible("message", !InMessage.empty());
	Panel->SetPartVisible("message_gap", !InMessage.empty());

	const float Scale = GetUiScale();
	const viewmodel::WidgetDef& Def = Panel->GetDef();
	const float ContentWidth = (Def.Rules.FixedSize.X - 2 * Def.Rules.Padding.X) * Scale;
	auto List = ButtonList::create();
	List->SetBelongsToOverlay(true);
	addChild(List);
	LayoutWidget* const Column = Panel->FindWidget("buttons");
	for ( const auto& Spec : InButtons )
	{
		auto CurrentButton = Button::create(Spec.first, cocos2d::Size(ContentWidth, Style().ButtonHeight * Scale), Spec.second);
		Column->AppendNode(CurrentButton);
		List->AddButton(CurrentButton);
	}
	Panel->setPosition(DesignWidth / 2, DesignHeight / 2);
	addChild(Panel);
	addChild(CreateControlHints(true), 1);
	return true;
}

} // namespace widgets
} // namespace tanks::view::cocos
