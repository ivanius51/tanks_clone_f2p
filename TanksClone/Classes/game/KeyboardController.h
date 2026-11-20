#pragma once

#include <vector>
#include "cocos2d.h"

namespace tanks
{

// Translates keyboard input into commands for the player object.
class KeyboardController
{
public:
	static KeyboardController& GetInstance();

private:
	KeyboardController();
	~KeyboardController() = default;
	KeyboardController(const KeyboardController&) = delete;
	KeyboardController& operator=(const KeyboardController&) = delete;

	void OnKeyPressed(cocos2d::EventKeyboard::KeyCode InKeyCode, cocos2d::Event* InEvent);
	void OnKeyReleased(cocos2d::EventKeyboard::KeyCode InKeyCode, cocos2d::Event* InEvent);

	// Movement keys currently held, in the order they were pressed.
	// The last one decides the direction.
	std::vector<cocos2d::EventKeyboard::KeyCode> HeldMoveKeys;
};

} // namespace tanks
