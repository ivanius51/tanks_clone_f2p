#pragma once

#include <functional>

#include "model/InputTypes.h"

namespace tanks::ports
{

/**
 * Port: delivers the player's key presses and releases. The adapter translates its platform's events
 * (Cocos keyboard events now; touch, gamepad or browser keys later) into model::KeyEvent.
 */
class IInputSource
{
public:
	using KeyHandler = std::function<void(const model::KeyEvent&)>;

	virtual ~IInputSource() = default;

	// Replaces the handler; an empty handler stops delivery.
	virtual void SetKeyHandler(const KeyHandler& InHandler) = 0;
};

} // namespace tanks::ports
