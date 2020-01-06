#include "KeyboardController.h"
#include "CommandsManager.h"

NS_CC_BEGIN
KeyboardController::KeyboardController()
{
	if ( !mKeyboardListener )
	{
		mKeyboardListener = EventListenerKeyboard::create();
		mKeyboardListener->onKeyPressed = CC_CALLBACK_2(KeyboardController::onKeyPressed, this);
		mKeyboardListener->onKeyReleased = CC_CALLBACK_2(KeyboardController::onKeyReleased, this);
		Director::getInstance()->getEventDispatcher()->addEventListenerWithFixedPriority(mKeyboardListener, -1);
	}
}

KeyboardController::~KeyboardController()
{
	if ( mKeyboardListener )
	{
		Director::getInstance()->getEventDispatcher()->removeEventListener(mKeyboardListener);
		mKeyboardListener = nullptr;
	}
}

void KeyboardController::onKeyPressed(EventKeyboard::KeyCode aKeyCode, Event* aEvent)
{
	//if ( mRegisteredKeyCodes.count(aKeyCode) )
	{
		++mKeysPressed;
		//COMMAND.addCommand(mRegisteredKeyCodes[aKeyCode], "PLAYER");
		switch ( aKeyCode )
		{
			case EventKeyboard::KeyCode::KEY_NONE:
				break;
			case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
			case EventKeyboard::KeyCode::KEY_A:
			case EventKeyboard::KeyCode::KEY_CAPITAL_A:
				COMMAND.addCommand(eCommandType::COMMAND_TYPE_MOVE_LEFT, "PLAYER");
				break;
			case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
			case EventKeyboard::KeyCode::KEY_D:
			case EventKeyboard::KeyCode::KEY_CAPITAL_D:
				COMMAND.addCommand(eCommandType::COMMAND_TYPE_MOVE_RIGHT, "PLAYER");
				break;
			case EventKeyboard::KeyCode::KEY_UP_ARROW:
			case EventKeyboard::KeyCode::KEY_W:
			case EventKeyboard::KeyCode::KEY_CAPITAL_W:
				COMMAND.addCommand(eCommandType::COMMAND_TYPE_MOVE_FORWARD, "PLAYER");
				break;
			case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
			case EventKeyboard::KeyCode::KEY_S:
			case EventKeyboard::KeyCode::KEY_CAPITAL_S:
				COMMAND.addCommand(eCommandType::COMMAND_TYPE_MOVE_BACK, "PLAYER");
				break;
			case EventKeyboard::KeyCode::KEY_SPACE:
				COMMAND.addCommand(eCommandType::COMMAND_TYPE_SHOOT, "PLAYER");
				break;
			default:
				break;
		}
	}
}

void KeyboardController::onKeyReleased(EventKeyboard::KeyCode aKeyCode, Event * aEvent)
{
	//if ( mRegisteredKeyCodes.count(aKeyCode) )
	{
		--mKeysPressed;
		switch ( aKeyCode )
		{
			case EventKeyboard::KeyCode::KEY_NONE:
				break;
			case EventKeyboard::KeyCode::KEY_LEFT_ARROW:
			case EventKeyboard::KeyCode::KEY_A:
			case EventKeyboard::KeyCode::KEY_CAPITAL_A:
				break;
			case EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
			case EventKeyboard::KeyCode::KEY_D:
			case EventKeyboard::KeyCode::KEY_CAPITAL_D:
				break;
			case EventKeyboard::KeyCode::KEY_UP_ARROW:
			case EventKeyboard::KeyCode::KEY_W:
			case EventKeyboard::KeyCode::KEY_CAPITAL_W:
				break;
			case EventKeyboard::KeyCode::KEY_DOWN_ARROW:
			case EventKeyboard::KeyCode::KEY_S:
			case EventKeyboard::KeyCode::KEY_CAPITAL_S:
				break;
			case EventKeyboard::KeyCode::KEY_SPACE:
				break;
			default:
				break;
		}
		if ( mKeysPressed == 0 )
		{
			COMMAND.addCommand(eCommandType::COMMAND_TYPE_STOP, "PLAYER");
		}
	}
}

KeyboardController& KeyboardController::getInstance()
{
	static KeyboardController instance;
	return instance;
}

void KeyboardController::registerPlayerKeyAction(EventKeyboard::KeyCode aCode, eCommandType aCommand)
{
	mRegisteredKeyCodes[aCode] = aCommand;
}

NS_CC_END