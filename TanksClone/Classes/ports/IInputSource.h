#pragma once


#include "model/InputTypes.h"
#include "templates/TFunction.h"

namespace game::ports
{
/**
 * Port: delivers the player's button presses and releases. The adapter translates its platform's events
 * (Cocos keyboard events now; touch, gamepad or browser keys later) into model::ButtonEvent; what a button means
 * is decided by the bindings, never by the adapter.
 */
class IInputSource
{
public:
	using ButtonHandler = TFunction<void(const model::ButtonEvent&)>;

	virtual ~IInputSource() = default;

	virtual void SetButtonHandler(const ButtonHandler& InHandler) = 0;
};
} // namespace game::ports
