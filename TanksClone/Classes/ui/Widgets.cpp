#include "ui/Widgets.h"
#include "templates/TFunction.h"

#include <cmath>
#include "templates/TContainers.h"

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "services/AudioManager.h"
#include "services/DisplaySettings.h"
#include "services/InputManager.h"
#include "services/ProfileManager.h"
#include "services/ResourceManager.h"
#include "services/StringsManager.h"
#include "services/UiStyle.h"
#include "ui/ActionListener.h"
#include "ui/CocosConvert.h"
#include "ui/FontAtlasHold.h"
#include "ui/GradientDraw.h"
#include "ui/ScreenFit.h"
#include "ui/LayoutWidget.h"
#include "ui/MenuBuilder.h"

namespace game::view::cocos
{
namespace widgets
{

namespace
{
	constexpr float SliderFillAlpha = 0.35f;
	constexpr float ButtonPressedShade = 0.6f;
	constexpr float SubtitleFontShare = 0.6f;
	constexpr float SubtitleMinFont = 13.0f;
	constexpr GLubyte LabelShadowAlpha = 170;
	constexpr float LabelShadowOffset = 2.0f;
	constexpr float GridStep = TileSize;

	const viewmodel::WidgetStyle& Style()
	{
		return GameContext::Get<UiStyle>().GetWidgets();
	}

	model::ResourceId FontId(const bool bInDecorative)
	{
		const FPersistentString Override = GameContext::Get<StringsManager>().GetFontId(bInDecorative);
		if ( Override.IsEmpty() )
		{
			return bInDecorative ? ids::fonts::Decorative : ids::fonts::Plain;
		}
		return model::ResourceId(Override.View());
	}

	std::string FontPath(const bool bInDecorative)
	{
		return std::string(GameContext::Get<ResourceManager>().ResolvePath(FontId(bInDecorative), model::ResourceKind::Font).View());
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
	return Style().TextScales.Get(GameContext::Get<ProfileManager>().Get().CurrentTextSize);
}

cocos2d::Size ScaleUi(const cocos2d::Size& InSize)
{
	return InSize * GetUiScale();
}

cocos2d::Label* CreateLabel(const std::string& InText, const float InFontSize, const cocos2d::Color3B& InColor, const bool bInDecorative)
{
	const float FontSize = InFontSize * GetUiScale();
	ResourceManager& Resources = GameContext::Get<ResourceManager>();
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
	CurrentLabel->enableShadow(cocos2d::Color4B(0, 0, 0, LabelShadowAlpha), cocos2d::Size(LabelShadowOffset, -LabelShadowOffset));
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
	cocos2d::SpriteFrame* const CurrentFrame = GameContext::Get<ResourceManager>().FindFrame(InFrame);
	return CurrentFrame ? cocos2d::Sprite::createWithSpriteFrame(CurrentFrame) : nullptr;
}

void AddBackKeyHandler(cocos2d::Node* const InOwner, const TFunction<void(), UiCallbackInlineSize>& InOnBack, const bool bInOwnerIsOverlay)
{
	const auto OnBack = MakeSharedValue<const TFunction<void(), UiCallbackInlineSize>>(InOnBack);
	AddActionHandler(InOwner, [OnBack, bInOwnerIsOverlay](const model::InputAction InAction)
	{
		if ( InAction == model::InputAction::UiBack && (bInOwnerIsOverlay || !Overlay::IsShown()) )
		{
			OnBack->Get()();
		}
	});
}

// ---------------------------------------------------------------- Button

Button* Button::create(const std::string& InText, const cocos2d::Size& InSize, const TFunction<void(), UiCallbackInlineSize>& InCallback)
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
	TSet<const cocos2d::Node*>& LiveButtons()
	{
		static TSet<const cocos2d::Node*> Buttons;
		return Buttons;
	}
}

Button* Button::FromNode(const Node* const InNode)
{
	return LiveButtons().Contains(InNode) ? static_cast<Button*>(const_cast<Node*>(InNode)) : nullptr;
}

Button::~Button()
{
	LiveButtons().Remove(this);
}

bool Button::init(const std::string& InText, const cocos2d::Size& InSize, const TFunction<void(), UiCallbackInlineSize>& InCallback)
{
	LiveButtons().Insert(this);
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
		PressedZone = ZoneAt(InTouch->getLocation());
		SlideTo(InTouch);
		Redraw();
		return true;
	};
	CurrentListener->onTouchMoved = [this](cocos2d::Touch* const InTouch, cocos2d::Event*)
	{
		if ( bPressed )
		{
			SlideTo(InTouch);
		}
	};
	CurrentListener->onTouchEnded = [this](cocos2d::Touch* const InTouch, cocos2d::Event*)
	{
		const bool bClick = bPressed && IsTouchInside(InTouch);
		const int Zone = bClick ? ZoneAt(InTouch->getLocation()) : 0;
		const int Started = PressedZone;
		bPressed = false;
		PressedZone = 0;
		Redraw();
		if ( CurrentControl == Control::Slider )
		{
			return;
		}
		if ( bClick && Zone != 0 && Zone == Started && AdjustCallback )
		{
			GameContext::Get<AudioManager>().Play(ids::sounds::UiClick);
			Adjust(Zone);
		}
		else if ( bClick )
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

	auto Mouse = cocos2d::EventListenerMouse::create();
	Mouse->onMouseMove = [this](cocos2d::Event* const InEvent)
	{
		if ( CurrentControl == Control::None )
		{
			return;
		}
		const auto Move = static_cast<cocos2d::EventMouse*>(InEvent);
		const int NewHover = bEnabled && IsShown() ? ZoneAt(cocos2d::Vec2(Move->getCursorX(), Move->getCursorY())) : 0;
		if ( NewHover != HoverZone )
		{
			HoverZone = NewHover;
			Redraw();
		}
	};
	_eventDispatcher->addEventListenerWithSceneGraphPriority(Mouse, this);

	Redraw();
	return true;
}

Button::Control Button::ControlFromKind(const std::string_view InKind)
{
	if ( InKind == "choice" )
	{
		return Control::Choice;
	}
	if ( InKind == "stepper" )
	{
		return Control::Stepper;
	}
	if ( InKind == "slider" )
	{
		return Control::Slider;
	}
	return InKind == "toggle" ? Control::Toggle : Control::None;
}

void Button::SetControl(const Control InControl, const bool bInChecked)
{
	CurrentControl = InControl;
	bChecked = bInChecked;
	UpdateContentSize();
	Redraw();
}

void Button::UpdateContentSize()
{
	const cocos2d::Size Full = getContentSize();
	const float Reserved = CurrentControl == Control::None ? 0.0f : 2.0f * Full.height;
	Content->SetFixedSize(cocos2d::Size(std::max(Full.width - Reserved, Full.height), Full.height));
	LayoutRow();
}

bool Button::IsUnderCursor(const cocos2d::Vec2& InWorld) const
{
	if ( !bEnabled || !IsShown() )
	{
		return false;
	}
	const cocos2d::Vec2 Local = convertToNodeSpace(InWorld);
	const cocos2d::Size Size = getContentSize();
	return cocos2d::Rect(-Size.width / 2, -Size.height / 2, Size.width, Size.height).containsPoint(Local);
}

int Button::ZoneAt(const cocos2d::Vec2& InWorld) const
{
	if ( CurrentControl == Control::None || CurrentControl == Control::Toggle )
	{
		return 0;
	}
	const cocos2d::Vec2 Local = convertToNodeSpace(InWorld);
	const cocos2d::Size Size = getContentSize();
	if ( Local.y < -Size.height / 2 || Local.y > Size.height / 2 )
	{
		return 0;
	}
	if ( Local.x < -Size.width / 2 + Size.height )
	{
		return Local.x < -Size.width / 2 ? 0 : -1;
	}
	return Local.x > Size.width / 2 - Size.height && Local.x <= Size.width / 2 ? 1 : 0;
}

void Button::SetText(const std::string& InText)
{
	Content->SetText("text", InText);
	if ( RowName )
	{
		const size_t Split = InText.find(": ");
		RowName->setString(Split == std::string::npos ? InText : InText.substr(0, Split));
		RowValue->setString(Split == std::string::npos ? std::string() : InText.substr(Split + 2));
		LayoutRow();
	}
}

void Button::SetRowMode(const bool bInRow)
{
	if ( bInRow == (RowName != nullptr) )
	{
		return;
	}
	Content->SetPartVisible("text", !bInRow);
	if ( !bInRow )
	{
		RowName->removeFromParent();
		RowValue->removeFromParent();
		RowName = nullptr;
		RowValue = nullptr;
		return;
	}
	RowName = CreateLabel("", RowFontSize);
	RowValue = CreateLabel("", RowFontSize);
	RowName->setAnchorPoint(cocos2d::Vec2(0.0f, 0.5f));
	RowValue->setAnchorPoint(cocos2d::Vec2(1.0f, 0.5f));
	addChild(RowName, 2);
	addChild(RowValue, 2);
	SetText(Content->GetText("text"));
	Redraw();
}

void Button::LayoutRow()
{
	if ( !RowName )
	{
		return;
	}
	const cocos2d::Size Size = getContentSize();
	const bool bHasZones = CurrentControl == Control::Choice || CurrentControl == Control::Stepper;
	const float Pad = Size.height * 0.3f;
	const float Left = -Size.width / 2 + Pad + (bHasZones || CurrentControl == Control::Toggle ? Size.height : 0.0f);
	const float Right = Size.width / 2 - Pad - (bHasZones ? Size.height : 0.0f);
	RowName->setPosition(Left, 0.0f);
	RowValue->setPosition(Right, 0.0f);
}

void Button::SetSize(const cocos2d::Size& InSize)
{
	setContentSize(InSize);
	UpdateContentSize();
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

void Button::SetActive(const bool bInActive)
{
	bActive = bInActive;
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
		GameContext::Get<AudioManager>().Play(ids::sounds::UiClick);
		// The callback may replace the scene and delete this button.
		auto CurrentCallback = Callback;
		CurrentCallback();
	}
}

viewmodel::ButtonStyle Button::GetEffectiveStyle() const
{
	viewmodel::ButtonStyle Result = Style().FindButtonStyle(StyleId);
	if ( MainColor )
	{
		Result.Fill = model::FlatGradient(*MainColor);
		Result.Pressed = Result.Fill.Scaled(ButtonPressedShade);
	}
	return Result;
}

void Button::SetStyle(const std::string_view InStyleId)
{
	StyleId = std::string(InStyleId);
	Redraw();
}

void Button::SetMainColor(const model::Color& InColor)
{
	MainColor = InColor;
	Redraw();
}

void Button::SetSubtitle(const std::string& InText)
{
	Subtitle = InText;
	Content->SetText("subtitle", InText);
	Content->SetPartVisible("subtitle", !InText.empty());
}

void Button::SetFontSize(const float InFontSize)
{
	viewmodel::WidgetDef Def = Content->GetDef();
	for ( viewmodel::WidgetChildDef& Current : Def.Children )
	{
		if ( Current.Id == "text" )
		{
			Current.FontSize = InFontSize;
		}
		else if ( Current.Id == "subtitle" )
		{
			Current.FontSize = std::max(InFontSize * SubtitleFontShare, SubtitleMinFont);
		}
	}
	const std::string Caption = Content->GetText("text");
	Content->removeFromParent();
	Content = LayoutWidget::Create(Def);
	addChild(Content, 1);
	UpdateContentSize();
	Content->SetText("text", Caption);
	SetSubtitle(Subtitle);
	Redraw();
}

void Button::Redraw()
{
	const cocos2d::Size CurrentSize = getContentSize();
	const viewmodel::ButtonStyle Look = GetEffectiveStyle();
	const model::Gradient& Fill = !bEnabled ? Look.Disabled : (bPressed ? Look.Pressed : Look.Fill);
	const model::Color Edge = bSelected && bEnabled ? Style().Colors.Selection : Look.Border;
	const cocos2d::Size Half = CurrentSize / 2;
	const float Border = Style().BorderWidth;

	Background->clear();
	FillGradientRect(Background, Half, Look.Radius, model::FlatGradient(Edge));
	FillGradientRect(Background, Half - cocos2d::Size(Border, Border), std::max(Look.Radius - Border, 0.0f), Fill);
	if ( bActive )
	{
		const float Thickness = Border * 2;
		Background->drawSolidRect(cocos2d::Vec2(Border - Half.width, Border - Half.height), cocos2d::Vec2(Half.width - Border, Border + Thickness - Half.height), cocos2d::Color4F(ToCocos(Style().Colors.Selection)));
	}
	Content->SetTextColor("text", bEnabled ? (bSelected || bActive ? colors::Accent() : ToCocos3B(Look.Text)) : colors::TextDim());
	if ( RowName )
	{
		RowName->setColor(bEnabled ? (bSelected ? colors::Accent() : ToCocos3B(Look.Text)) : colors::TextDim());
		RowValue->setColor(bEnabled ? colors::Accent() : colors::TextDim());
	}
	DrawControls();
}

void Button::DrawControls()
{
	if ( CurrentControl == Control::None )
	{
		return;
	}
	const cocos2d::Size Size = getContentSize();
	if ( CurrentControl == Control::Slider )
	{
		const cocos2d::Color4F Bar = bEnabled ? colors::Selection() : cocos2d::Color4F(colors::TextDim());
		Background->drawSolidRect(cocos2d::Vec2(-Size.width / 2 + 1, -Size.height / 2 + 1), cocos2d::Vec2(-Size.width / 2 + 1 + (Size.width - 2) * FillFraction, Size.height / 2 - 1), cocos2d::Color4F(Bar.r, Bar.g, Bar.b, SliderFillAlpha));
		return;
	}
	const float Zone = Size.height;
	const float Half = Zone * 0.22f;
	const float Thickness = std::max(1.0f, Zone * 0.05f);
	const auto DrawZone = [&](const int InSide, const float InCenterX)
	{
		const bool bHot = bEnabled && HoverZone == InSide;
		if ( bHot )
		{
			const cocos2d::Color4F Hover = PressedZone == InSide ? colors::ButtonPressed() : colors::Selection();
			Background->drawSolidRect(cocos2d::Vec2(InCenterX - Zone / 2 + 1, -Size.height / 2 + 1), cocos2d::Vec2(InCenterX + Zone / 2 - 1, Size.height / 2 - 1),
				cocos2d::Color4F(Hover.r, Hover.g, Hover.b, 0.35f));
		}
		const cocos2d::Color4F Ink = cocos2d::Color4F(!bEnabled ? colors::TextDim() : (bHot ? colors::Accent() : colors::TextNormal()));
		const float Side = static_cast<float>(InSide);
		switch ( CurrentControl )
		{
		case Control::Choice:
			Background->drawSegment(cocos2d::Vec2(InCenterX - Side * Half, Half * 1.6f), cocos2d::Vec2(InCenterX + Side * Half, 0), Thickness, Ink);
			Background->drawSegment(cocos2d::Vec2(InCenterX + Side * Half, 0), cocos2d::Vec2(InCenterX - Side * Half, -Half * 1.6f), Thickness, Ink);
			break;
		case Control::Stepper:
			Background->drawSegment(cocos2d::Vec2(InCenterX - Half * 1.2f, 0), cocos2d::Vec2(InCenterX + Half * 1.2f, 0), Thickness, Ink);
			if ( InSide > 0 )
			{
				Background->drawSegment(cocos2d::Vec2(InCenterX, -Half * 1.2f), cocos2d::Vec2(InCenterX, Half * 1.2f), Thickness, Ink);
			}
			break;
		default:
			break;
		}
	};
	if ( CurrentControl == Control::Toggle )
	{
		const float CenterX = -Size.width / 2 + Zone / 2;
		const float Box = Half * 1.7f;
		const cocos2d::Color4F Ink = cocos2d::Color4F(bEnabled ? colors::TextNormal() : colors::TextDim());
		Background->drawRect(cocos2d::Vec2(CenterX - Box, -Box), cocos2d::Vec2(CenterX + Box, Box), Ink);
		if ( bChecked )
		{
			const float Inner = Box * 0.6f;
			Background->drawSegment(cocos2d::Vec2(CenterX - Inner, -Inner), cocos2d::Vec2(CenterX + Inner, Inner), Thickness, Ink);
			Background->drawSegment(cocos2d::Vec2(CenterX - Inner, Inner), cocos2d::Vec2(CenterX + Inner, -Inner), Thickness, Ink);
		}
		return;
	}
	DrawZone(-1, -Size.width / 2 + Zone / 2);
	DrawZone(1, Size.width / 2 - Zone / 2);
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

void Button::SetSlide(const TFunction<void(float), UiCallbackInlineSize>& InSlide)
{
	SlideCallback = InSlide;
}

void Button::SetFillFraction(const float InFraction)
{
	FillFraction = std::clamp(InFraction, 0.0f, 1.0f);
	Redraw();
}

void Button::SlideTo(const cocos2d::Touch* const InTouch)
{
	if ( CurrentControl != Control::Slider || !SlideCallback )
	{
		return;
	}
	const float Width = getContentSize().width;
	SlideCallback(std::clamp((convertToNodeSpace(InTouch->getLocation()).x + Width / 2) / Width, 0.0f, 1.0f));
}

void Button::SetAdjust(const TFunction<void(int), UiCallbackInlineSize>& InAdjust)
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
				if ( Selected >= 0 && Selected < static_cast<int>(Buttons.Size()) )
				{
					Buttons[Selected]->Adjust(InAction == model::InputAction::UiLeft ? -1 : 1);
				}
				break;
			case model::InputAction::UiConfirm:
				if ( Selected >= 0 && Selected < static_cast<int>(Buttons.Size()) )
				{
					Buttons[Selected]->Activate();
				}
				break;
			default:
				break;
		}
	});

	auto Mouse = cocos2d::EventListenerMouse::create();
	Mouse->onMouseMove = [this](cocos2d::Event* const InEvent)
	{
		if ( Overlay::IsShown() && !bBelongsToOverlay )
		{
			return;
		}
		const auto Move = static_cast<cocos2d::EventMouse*>(InEvent);
		const cocos2d::Vec2 Cursor(Move->getCursorX(), Move->getCursorY());
		for ( int Index = 0; Index < static_cast<int>(Buttons.Size()); ++Index )
		{
			if ( Buttons[Index]->IsUnderCursor(Cursor) )
			{
				if ( Index != Selected )
				{
					Select(Index);
				}
				return;
			}
		}
	};
	_eventDispatcher->addEventListenerWithSceneGraphPriority(Mouse, this);
	return true;
}

void ButtonList::AddButton(Button* const InButton)
{
	Buttons.PushBack(InButton);
	if ( Buttons.Size() == 1 )
	{
		Select(0);
	}
}

void ButtonList::LayoutVertically(const cocos2d::Vec2& InCenter, const float InSpacing)
{
	if ( Buttons.IsEmpty() )
	{
		return;
	}

	const float CurrentStep = Buttons[0]->getContentSize().height + InSpacing;
	float Y = InCenter.y + CurrentStep * (Buttons.Size() - 1) / 2;
	for ( auto CurrentButton : Buttons )
	{
		CurrentButton->setPosition(InCenter.x, Y);
		Y -= CurrentStep;
	}
}

void ButtonList::Select(const int InIndex)
{
	Selected = InIndex;
	for ( int Index = 0; Index < static_cast<int>(Buttons.Size()); ++Index )
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
	Buttons.Clear();
	Selected = 0;
}

void ButtonList::MoveSelection(const int InStep)
{
	const int CurrentCount = static_cast<int>(Buttons.Size());
	for ( int Index = 1; Index <= CurrentCount; ++Index )
	{
		const int Candidate = ((Selected + InStep * Index) % CurrentCount + CurrentCount) % CurrentCount;
		if ( Buttons[Candidate]->IsEnabled() )
		{
			if ( Candidate != Selected )
			{
				GameContext::Get<AudioManager>().Play(ids::sounds::UiMove);
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

namespace
{
	int GTabsHintOwners = 0;
}

void SetTabsHint(const bool bInShown)
{
	GTabsHintOwners = std::max(0, GTabsHintOwners + (bInShown ? 1 : -1));
}

cocos2d::Node* CreateControlHints(const bool bInForOverlay)
{
	LayoutWidget* const Hints = LayoutWidget::Create(ids::widgets::ControlHints);
	if ( !Hints )
	{
		return cocos2d::Node::create();
	}
	const auto Refresh = [Hints, bInForOverlay](float)
	{
		const InputManager& Input = GameContext::Get<InputManager>();
		const StringsManager& Strings = GameContext::Get<StringsManager>();
		const auto Show = [&](const char* const InChild, const model::InputAction InAction, const char* const InTextId)
		{
			LayoutWidget* const Hint = Hints->FindWidget(InChild);
			Hint->FindWidget("key")->SetText("text", Input.GetButtonLabel(model::InputContext::Menu, InAction));
			Hint->SetText("text", Strings.GetString(InTextId));
		};
		Show("select", model::InputAction::UiConfirm, "HINT_SELECT");
		Show("back", model::InputAction::UiBack, "HINT_BACK");
		Show("change", model::InputAction::UiRight, "HINT_CHANGE");
		Show("tabs", model::InputAction::NextTab, "HINT_TABS");
		Hints->FindWidget("tabs")->FindWidget("key")->SetText("text", Input.GetButtonLabel(model::InputContext::Menu, model::InputAction::PrevTab));
		Hints->FindWidget("tabs")->FindWidget("key_right")->SetText("text", Input.GetButtonLabel(model::InputContext::Menu, model::InputAction::NextTab));
		Hints->FindWidget("tabs")->setVisible(GTabsHintOwners > 0);
		Hints->FindWidget("change")->FindWidget("key")->SetText("text", Input.GetButtonLabel(model::InputContext::Menu, model::InputAction::UiLeft));
		Hints->FindWidget("change")->FindWidget("key_right")->SetText("text", Input.GetButtonLabel(model::InputContext::Menu, model::InputAction::UiRight));
		Hints->setPosition(DesignWidth / 2, Style().HintsMargin + Hints->getContentSize().height / 2);
		Hints->setVisible(bInForOverlay || !Overlay::IsShown());
	};
	Refresh(0.0f);
	Hints->schedule(Refresh, "control_hints");
	if ( !bInForOverlay )
	{
		return Hints;
	}
	cocos2d::Node* const Result = cocos2d::Node::create();
	Result->addChild(LayoutWidget::Create(ids::widgets::HintsBar));
	Result->addChild(Hints, 1);
	return Result;
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

Overlay* Overlay::create(const std::string& InTitle, const std::string& InMessage, const TArray<ButtonSpec>& InButtons, const float InButtonHeight)
{
	Overlay* NewOverlay = new (std::nothrow) Overlay();
	if ( NewOverlay && NewOverlay->init(InTitle, InMessage, InButtons, InButtonHeight) )
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

Button* CreateEntryButton(const MenuEntry& InEntry, const cocos2d::Size& InDefaultSize)
{
	const viewmodel::MenuItemDef* const Item = InEntry.Item;
	cocos2d::Size Size = InDefaultSize;
	if ( Item && Item->Width > 0.0f )
	{
		Size.width = Item->Width * GetUiScale();
	}
	if ( Item && Item->Height > 0.0f )
	{
		Size.height = Item->Height * GetUiScale();
	}
	Button* const Created = Button::create(InEntry.Caption, Size, InEntry.Action);
	if ( !Item )
	{
		return Created;
	}
	Created->SetStyle(Item->Style);
	if ( Item->FontSize > 0.0f )
	{
		Created->SetFontSize(Item->FontSize);
	}
	Created->SetSubtitle(InEntry.Subtitle);
	Created->SetEnabled(InEntry.bEnabled);
	return Created;
}

viewmodel::MenuButtonMetrics GetMenuButtonMetrics(const viewmodel::MenuDef* const InMenu, const std::string_view InStyle)
{
	const viewmodel::MenuLayout& Layout = GameContext::Get<UiStyle>().GetMenu();
	viewmodel::MenuButtonMetrics Result;
	if ( InStyle == "main" )
	{
		Result = { Layout.Main.ButtonWidth, Layout.Main.ButtonHeight, Layout.Main.ButtonSpacing };
	}
	else if ( InStyle == "settings" )
	{
		Result = { Layout.Settings.ButtonWidth, Layout.Settings.ButtonHeight, Layout.Settings.ButtonSpacing };
	}
	else
	{
		Result.Height = Style().ButtonHeight;
	}
	if ( InMenu )
	{
		Result.Width = InMenu->Button.Width > 0.0f ? InMenu->Button.Width : Result.Width;
		Result.Height = InMenu->Button.Height > 0.0f ? InMenu->Button.Height : Result.Height;
		Result.Spacing = InMenu->Button.Spacing > 0.0f ? InMenu->Button.Spacing : Result.Spacing;
	}
	return Result;
}

cocos2d::Size GetMenuButtonSize(const viewmodel::MenuDef& InMenu, const LayoutWidget& InPanel)
{
	const viewmodel::MenuButtonMetrics Metrics = GetMenuButtonMetrics(&InMenu, InMenu.ButtonStyle);
	if ( InMenu.ButtonStyle == "main" || InMenu.ButtonStyle == "settings" )
	{
		return ScaleUi(cocos2d::Size(Metrics.Width, Metrics.Height));
	}
	const viewmodel::WidgetDef& Def = InPanel.GetDef();
	const float Scale = GetUiScale();
	return cocos2d::Size((Def.Rules.FixedSize.X - 2 * Def.Rules.Padding.X) * Scale, Metrics.Height * Scale);
}

Overlay* Overlay::CreateFromMenu(const std::string_view InMenuId, const MenuBindings& InBindings)
{
	const viewmodel::MenuDef* const Menu = GameContext::Get<UiStyle>().FindMenu(InMenuId);
	if ( !Menu )
	{
		return nullptr;
	}
	const auto TextOf = [Menu](const char* const InChild)
	{
		const viewmodel::MenuTextDef* const Found = Menu->FindTextDef(InChild);
		return Found ? MenuText(*Found) : std::string();
	};
	TArray<ButtonSpec> Buttons;
	for ( const MenuEntry& Entry : ResolveMenuEntries(*Menu, InBindings, false) )
	{
		Buttons.PushBack({ Entry.Caption, Entry.Action });
	}
	return create(TextOf("title"), TextOf("message"), Buttons, GetMenuButtonMetrics(Menu, Menu->ButtonStyle).Height);
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

bool Overlay::init(const std::string& InTitle, const std::string& InMessage, const TArray<ButtonSpec>& InButtons, const float InButtonHeight)
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
		auto CurrentButton = Button::create(Spec.first, cocos2d::Size(ContentWidth, (InButtonHeight > 0.0f ? InButtonHeight : Style().ButtonHeight) * Scale), Spec.second);
		Column->AppendNode(CurrentButton);
		List->AddButton(CurrentButton);
	}
	Panel->setPosition(DesignWidth / 2, DesignHeight / 2);
	addChild(Panel);
	addChild(CreateControlHints(true), 1);
	return true;
}

} // namespace widgets
} // namespace game::view::cocos
