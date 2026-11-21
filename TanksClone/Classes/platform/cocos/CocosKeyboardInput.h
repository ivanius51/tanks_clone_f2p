#pragma once

#include "cocos2d.h"

#include "ports/IInputSource.h"

namespace tanks::view::cocos
{

/** IInputSource over the engine's keyboard events. */
class CocosKeyboardInput final : public ports::IInputSource
{
public:
	CocosKeyboardInput();
	~CocosKeyboardInput() override;
	CocosKeyboardInput(const CocosKeyboardInput&) = delete;
	CocosKeyboardInput& operator=(const CocosKeyboardInput&) = delete;

	void SetKeyHandler(const KeyHandler& InHandler) override;

	static model::Key ToKey(cocos2d::EventKeyboard::KeyCode InKeyCode);

private:
	KeyHandler Handler;
	cocos2d::EventListenerKeyboard* KeyListener = nullptr;
};

} // namespace tanks::view::cocos
