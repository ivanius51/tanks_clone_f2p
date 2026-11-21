#pragma once

#include <functional>

#include "model/InputTypes.h"

namespace tanks::ports
{

/**
 * Port: delivers the player's button presses and releases. The adapter translates its platform's events
 * (Cocos keyboard events now; touch, gamepad or browser keys later) into model::ButtonEvent; what a button means
 * is decided by the bindings, never by the adapter.
 */
class IInputSource
{
public:
	using ButtonHandler = std::function<void(const model::ButtonEvent&)>;

	virtual ~IInputSource() = default;

	// Replaces the handler; an empty handler stops delivery.
	virtual void SetButtonHandler(const ButtonHandler& InHandler) = 0;
};

} // namespace tanks::ports
