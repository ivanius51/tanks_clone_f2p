#include "platform/cocos/CocosKeyboardInput.h"

namespace tanks::view::cocos
{

namespace
{
	constexpr int ListenerPriority = -1;

	using Code = cocos2d::EventKeyboard::KeyCode;

	// The engine lists lower and capital letters, digits and F-keys as contiguous ranges.
	static_assert(static_cast<int>(Code::KEY_Z) - static_cast<int>(Code::KEY_A) == 25, "engine letters are in order");
	static_assert(static_cast<int>(Code::KEY_CAPITAL_Z) - static_cast<int>(Code::KEY_CAPITAL_A) == 25, "engine capitals are in order");
	static_assert(static_cast<int>(Code::KEY_9) - static_cast<int>(Code::KEY_0) == 9, "engine digits are in order");
	static_assert(static_cast<int>(Code::KEY_F12) - static_cast<int>(Code::KEY_F1) == 11, "engine F-keys are in order");

	bool TryRange(const Code InCode, const Code InFirst, const Code InLast, const model::Key InFirstKey, model::Key& OutKey)
	{
		if ( InCode < InFirst || InCode > InLast )
		{
			return false;
		}
		OutKey = static_cast<model::Key>(static_cast<int>(InFirstKey) + static_cast<int>(InCode) - static_cast<int>(InFirst));
		return true;
	}
}

CocosKeyboardInput::CocosKeyboardInput()
{
	KeyListener = cocos2d::EventListenerKeyboard::create();
	KeyListener->onKeyPressed = [this](const Code InKeyCode, cocos2d::Event*) { Send(InKeyCode, true); };
	KeyListener->onKeyReleased = [this](const Code InKeyCode, cocos2d::Event*) { Send(InKeyCode, false); };
	cocos2d::Director::getInstance()->getEventDispatcher()->addEventListenerWithFixedPriority(KeyListener, ListenerPriority);
}

CocosKeyboardInput::~CocosKeyboardInput()
{
	cocos2d::Director::getInstance()->getEventDispatcher()->removeEventListener(KeyListener);
}

void CocosKeyboardInput::SetButtonHandler(const ButtonHandler& InHandler)
{
	Handler = InHandler;
}

void CocosKeyboardInput::Send(const Code InKeyCode, const bool bInPressed) const
{
	const model::Key CurrentKey = ToKey(InKeyCode);
	if ( Handler && CurrentKey != model::Key::Other )
	{
		Handler(model::ButtonEvent{ model::InputButton::FromKey(CurrentKey), bInPressed });
	}
}

model::Key CocosKeyboardInput::ToKey(const Code InKeyCode)
{
	model::Key Result = model::Key::Other;
	if ( TryRange(InKeyCode, Code::KEY_A, Code::KEY_Z, model::Key::A, Result)
		|| TryRange(InKeyCode, Code::KEY_CAPITAL_A, Code::KEY_CAPITAL_Z, model::Key::A, Result)
		|| TryRange(InKeyCode, Code::KEY_0, Code::KEY_9, model::Key::Digit0, Result)
		|| TryRange(InKeyCode, Code::KEY_F1, Code::KEY_F12, model::Key::F1, Result) )
	{
		return Result;
	}
	switch ( InKeyCode )
	{
		case Code::KEY_LEFT_ARROW: return model::Key::Left;
		case Code::KEY_RIGHT_ARROW: return model::Key::Right;
		case Code::KEY_UP_ARROW: return model::Key::Up;
		case Code::KEY_DOWN_ARROW: return model::Key::Down;
		case Code::KEY_ENTER: return model::Key::Enter;
		case Code::KEY_KP_ENTER: return model::Key::KeypadEnter;
		case Code::KEY_ESCAPE: return model::Key::Escape; // also the phone's Back (same engine code)
		case Code::KEY_TAB: return model::Key::Tab;
		case Code::KEY_SPACE: return model::Key::Space;
		case Code::KEY_BACKSPACE: return model::Key::Backspace;
		case Code::KEY_LEFT_SHIFT:
		case Code::KEY_RIGHT_SHIFT: return model::Key::Shift;
		case Code::KEY_LEFT_CTRL:
		case Code::KEY_RIGHT_CTRL: return model::Key::Ctrl;
		default: return model::Key::Other;
	}
}

} // namespace tanks::view::cocos
