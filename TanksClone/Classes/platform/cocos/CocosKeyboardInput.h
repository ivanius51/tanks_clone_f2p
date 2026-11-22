#pragma once

#include "cocos2d.h"

#include "platform/win32/XInputGamepad.h"
#include "ports/IInputSource.h"

namespace tanks::view::cocos
{

/** IInputSource over the engine's keyboard events (engine key codes into keyboard buttons) and XInput gamepads polled every frame. */
class CocosKeyboardInput final : public ports::IInputSource
{
public:
	CocosKeyboardInput();
	~CocosKeyboardInput() override;
	CocosKeyboardInput(const CocosKeyboardInput&) = delete;
	CocosKeyboardInput& operator=(const CocosKeyboardInput&) = delete;

	void SetButtonHandler(const ButtonHandler& InHandler) override;

	static model::Key ToKey(cocos2d::EventKeyboard::KeyCode InKeyCode);

private:
	void Send(cocos2d::EventKeyboard::KeyCode InKeyCode, bool bInPressed) const;

	ButtonHandler Handler;
	win32::XInputGamepad Gamepads;
	cocos2d::EventListenerKeyboard* KeyListener = nullptr;
	cocos2d::EventListenerCustom* PollListener = nullptr;
};

} // namespace tanks::view::cocos
