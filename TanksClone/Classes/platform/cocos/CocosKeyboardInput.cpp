#include "platform/cocos/CocosKeyboardInput.h"

namespace tanks::view::cocos
{

namespace
{
	constexpr int ListenerPriority = -1;
}

CocosKeyboardInput::CocosKeyboardInput()
{
	KeyListener = cocos2d::EventListenerKeyboard::create();
	KeyListener->onKeyPressed = [this](const cocos2d::EventKeyboard::KeyCode InKeyCode, cocos2d::Event*)
	{
		if ( Handler )
		{
			Handler(model::KeyEvent{ ToKey(InKeyCode), true });
		}
	};
	KeyListener->onKeyReleased = [this](const cocos2d::EventKeyboard::KeyCode InKeyCode, cocos2d::Event*)
	{
		if ( Handler )
		{
			Handler(model::KeyEvent{ ToKey(InKeyCode), false });
		}
	};
	cocos2d::Director::getInstance()->getEventDispatcher()->addEventListenerWithFixedPriority(KeyListener, ListenerPriority);
}

CocosKeyboardInput::~CocosKeyboardInput()
{
	cocos2d::Director::getInstance()->getEventDispatcher()->removeEventListener(KeyListener);
}

void CocosKeyboardInput::SetKeyHandler(const KeyHandler& InHandler)
{
	Handler = InHandler;
}

model::Key CocosKeyboardInput::ToKey(const cocos2d::EventKeyboard::KeyCode InKeyCode)
{
	using Code = cocos2d::EventKeyboard::KeyCode;
	switch ( InKeyCode )
	{
		case Code::KEY_LEFT_ARROW: return model::Key::Left;
		case Code::KEY_RIGHT_ARROW: return model::Key::Right;
		case Code::KEY_UP_ARROW: return model::Key::Up;
		case Code::KEY_DOWN_ARROW: return model::Key::Down;
		case Code::KEY_A:
		case Code::KEY_CAPITAL_A: return model::Key::A;
		case Code::KEY_D:
		case Code::KEY_CAPITAL_D: return model::Key::D;
		case Code::KEY_W:
		case Code::KEY_CAPITAL_W: return model::Key::W;
		case Code::KEY_S:
		case Code::KEY_CAPITAL_S: return model::Key::S;
		case Code::KEY_E:
		case Code::KEY_CAPITAL_E: return model::Key::E;
		case Code::KEY_Q:
		case Code::KEY_CAPITAL_Q: return model::Key::Q;
		case Code::KEY_TAB: return model::Key::Tab;
		case Code::KEY_SPACE: return model::Key::Space;
		case Code::KEY_ESCAPE: return model::Key::Escape;
		default: return model::Key::Other;
	}
}

} // namespace tanks::view::cocos
