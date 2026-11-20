#include "ui/Widgets.h"

#include <unordered_set>

#include <algorithm>
#include "core/GameConstants.h"
#include "services/DisplaySettings.h"

namespace tanks::view::cocos
{
namespace widgets
{

namespace
{
	constexpr float BorderWidth = 2.0f;
	constexpr float ButtonHeight = 52.0f;
	constexpr float OverlayWidth = 640.0f;
	constexpr float OverlayPadding = 28.0f;
	constexpr float OverlayButtonSpacing = 12.0f;
	constexpr float ButtonFontSize = 24.0f;
	constexpr float TitleFontSize = 40.0f;
	constexpr float MessageFontSize = 22.0f;
	constexpr float GridStep = TileSize;
	// The pixel font is much wider than the plain one at the same size.
	constexpr float DecorativeFontScale = 0.55f;

	void DrawBorderedRect(cocos2d::DrawNode* const InNode, const cocos2d::Size& InSize, const cocos2d::Color4F& InFill, const cocos2d::Color4F& InBorder)
	{
		const cocos2d::Vec2 Low(-InSize.width / 2, -InSize.height / 2);
		const cocos2d::Vec2 High(InSize.width / 2, InSize.height / 2);
		InNode->drawSolidRect(Low, High, InBorder);
		InNode->drawSolidRect(Low + cocos2d::Vec2(BorderWidth, BorderWidth), High - cocos2d::Vec2(BorderWidth, BorderWidth), InFill);
	}
}

cocos2d::Label* CreateLabel(const std::string& InText, const float InFontSize, const cocos2d::Color3B& InColor, const bool bInDecorative)
{
	cocos2d::Label* CurrentLabel = cocos2d::Label::createWithTTF(InText, bInDecorative ? UiFont : UiFontPlain, bInDecorative ? InFontSize * DecorativeFontScale : InFontSize);
	if ( !CurrentLabel )
	{
		CurrentLabel = cocos2d::Label::createWithSystemFont(InText, "Arial", InFontSize);
	}
	else
	{
		auto* const ScaleListener = cocos2d::EventListenerCustom::create(ContentScaleChangedEvent, [CurrentLabel](cocos2d::EventCustom*)
		{
			CurrentLabel->setTTFConfig(CurrentLabel->getTTFConfig());
		});
		CurrentLabel->getEventDispatcher()->addEventListenerWithSceneGraphPriority(ScaleListener, CurrentLabel);
	}
	CurrentLabel->setColor(InColor);
	return CurrentLabel;
}

cocos2d::Node* CreateScreenBackground()
{
	auto CurrentRoot = cocos2d::LayerColor::create(cocos2d::Color4B(14, 17, 24, 255), DesignWidth, DesignHeight);

	auto CurrentGrid = cocos2d::DrawNode::create();
	const cocos2d::Color4F Line(1.0f, 1.0f, 1.0f, 0.04f);
	for ( float X = 0; X <= DesignWidth; X += GridStep )
	{
		CurrentGrid->drawLine(cocos2d::Vec2(X, 0), cocos2d::Vec2(X, DesignHeight), Line);
	}
	for ( float Y = 0; Y <= DesignHeight; Y += GridStep )
	{
		CurrentGrid->drawLine(cocos2d::Vec2(0, Y), cocos2d::Vec2(DesignWidth, Y), Line);
	}
	CurrentRoot->addChild(CurrentGrid);
	return CurrentRoot;
}

cocos2d::Sprite* CreateFrameSprite(const std::string& InFrame)
{
	cocos2d::SpriteFrame* CurrentFrame = cocos2d::SpriteFrameCache::getInstance()->getSpriteFrameByName(InFrame);
	return CurrentFrame ? cocos2d::Sprite::createWithSpriteFrame(CurrentFrame) : nullptr;
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

	TextLabel = CreateLabel(InText, ButtonFontSize);
	addChild(TextLabel, 1);

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
	TextLabel->setString(InText);
}

std::string Button::GetText() const
{
	return TextLabel->getString();
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
		// The callback may replace the scene and delete this button.
		auto CurrentCallback = Callback;
		CurrentCallback();
	}
}

void Button::Redraw()
{
	const cocos2d::Size CurrentSize = getContentSize();
	const cocos2d::Color4F& Fill = !bEnabled ? colors::ButtonDisabled : (bPressed ? colors::ButtonPressed : colors::ButtonColor);

	Background->clear();
	DrawBorderedRect(Background, CurrentSize, Fill, bSelected && bEnabled ? colors::Selection : colors::ButtonBorder);
	TextLabel->setColor(bEnabled ? (bSelected ? colors::Accent : colors::TextNormal) : colors::TextDim);
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

// ------------------------------------------------------------ ButtonList

bool ButtonList::init()
{
	if ( !Node::init() )
	{
		return false;
	}

	auto CurrentListener = cocos2d::EventListenerKeyboard::create();
	CurrentListener->onKeyPressed = [this](const cocos2d::EventKeyboard::KeyCode InKeyCode, cocos2d::Event*)
	{
		if ( Overlay::IsShown() && !bBelongsToOverlay )
		{
			return;
		}
		switch ( InKeyCode )
		{
			case cocos2d::EventKeyboard::KeyCode::KEY_UP_ARROW:
			case cocos2d::EventKeyboard::KeyCode::KEY_W:
				MoveSelection(-1);
				break;
			case cocos2d::EventKeyboard::KeyCode::KEY_DOWN_ARROW:
			case cocos2d::EventKeyboard::KeyCode::KEY_S:
				MoveSelection(1);
				break;
			case cocos2d::EventKeyboard::KeyCode::KEY_ENTER:
			case cocos2d::EventKeyboard::KeyCode::KEY_KP_ENTER:
			case cocos2d::EventKeyboard::KeyCode::KEY_SPACE:
				if ( Selected >= 0 && Selected < static_cast<int>(Buttons.size()) )
				{
					Buttons[Selected]->Activate();
				}
				break;
			default:
				break;
		}
	};
	_eventDispatcher->addEventListenerWithSceneGraphPriority(CurrentListener, this);
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

bool Overlay::init(const std::string& InTitle, const std::string& InMessage, const std::vector<ButtonSpec>& InButtons)
{
	if ( !LayerColor::initWithColor(cocos2d::Color4B(0, 0, 0, 170), DesignWidth, DesignHeight) )
	{
		return false;
	}

	// Block everything underneath.
	auto Blocker = cocos2d::EventListenerTouchOneByOne::create();
	Blocker->setSwallowTouches(true);
	Blocker->onTouchBegan = [](cocos2d::Touch*, cocos2d::Event*) { return true; };
	_eventDispatcher->addEventListenerWithSceneGraphPriority(Blocker, this);

	auto Title = CreateLabel(InTitle, TitleFontSize, colors::Accent, true);
	const float ContentWidth = OverlayWidth - 2 * OverlayPadding;

	cocos2d::Label* Message = nullptr;
	if ( !InMessage.empty() )
	{
		Message = CreateLabel(InMessage, MessageFontSize);
		Message->setAlignment(cocos2d::TextHAlignment::CENTER);
		Message->setDimensions(ContentWidth, 0);
	}

	const float MessageHeight = Message ? Message->getContentSize().height : 0.0f;
	const float ButtonsHeight = InButtons.size() * (ButtonHeight + OverlayButtonSpacing);
	const float CurrentHeight = OverlayPadding * 2 + Title->getContentSize().height + (Message ? 20.0f + MessageHeight : 0.0f)
		+ 24.0f + ButtonsHeight;

	const cocos2d::Vec2 Center(DesignWidth / 2, DesignHeight / 2);
	auto CurrentPanel = cocos2d::DrawNode::create();
	CurrentPanel->setPosition(Center);
	DrawBorderedRect(CurrentPanel, cocos2d::Size(OverlayWidth, CurrentHeight), colors::Panel, colors::PanelBorder);
	addChild(CurrentPanel);

	float Y = Center.y + CurrentHeight / 2 - OverlayPadding - Title->getContentSize().height / 2;
	Title->setPosition(Center.x, Y);
	addChild(Title, 1);
	Y -= Title->getContentSize().height / 2;

	if ( Message )
	{
		Y -= 20.0f + MessageHeight / 2;
		Message->setPosition(Center.x, Y);
		addChild(Message, 1);
		Y -= MessageHeight / 2;
	}

	auto List = ButtonList::create();
	List->SetBelongsToOverlay(true);
	addChild(List);
	Y -= 24.0f + ButtonHeight / 2;
	for ( const auto& Spec : InButtons )
	{
		auto CurrentButton = Button::create(Spec.first, cocos2d::Size(ContentWidth, ButtonHeight), Spec.second);
		CurrentButton->setPosition(Center.x, Y);
		addChild(CurrentButton, 1);
		List->AddButton(CurrentButton);
		Y -= ButtonHeight + OverlayButtonSpacing;
	}
	return true;
}

} // namespace widgets
} // namespace tanks::view::cocos