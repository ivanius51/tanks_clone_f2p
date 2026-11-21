#pragma once

#include "cocos2d.h"

#include "ports/IInputSource.h"

namespace tanks::view::cocos
{

/** IInputSource over the engine's keyboard events: converts engine key codes into keyboard buttons. */
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
	cocos2d::EventListenerKeyboard* KeyListener = nullptr;
};

} // namespace tanks::view::cocos
