#ifndef _KeyboardController_H_
#define _KeyboardController_H_

#include "cocos2d.h"
#include "GameCommand.h"

NS_CC_BEGIN

class KeyboardController
{
private:
	KeyboardController();
	~KeyboardController();
	KeyboardController(const KeyboardController&);
	KeyboardController& operator=(const KeyboardController&);

	EventKeyboard::KeyCode mLastMoveKey;
	unsigned mKeysPressed;

	EventListenerKeyboard* mKeyboardListener;
	void onKeyPressed(EventKeyboard::KeyCode aKeyCode, Event* aEvent);
	void onKeyReleased(EventKeyboard::KeyCode aKeyCode, Event* aEvent);

	std::map< EventKeyboard::KeyCode, eCommandType > mRegisteredKeyCodes;

public:
	static KeyboardController& getInstance();

	void registerPlayerKeyAction(EventKeyboard::KeyCode aCode, eCommandType aCommand);
};

#define KEYBOARD KeyboardController::getInstance()

NS_CC_END
#endif
