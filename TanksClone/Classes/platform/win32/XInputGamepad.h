#pragma once

#include <array>

#include "ports/IInputSource.h"

namespace tanks::view::win32
{

/**
 * Reads up to four XInput gamepads and reports button changes as model::ButtonEvent (pad player 1-4).
 * The left stick acts as the d-pad and the triggers as buttons; what they mean is up to configs/input.json.
 */
class XInputGamepad
{
public:
	// Compares every pad with the last poll and sends the buttons that changed.
	void Poll(const ports::IInputSource::ButtonHandler& InHandler);

private:
	std::array<unsigned int, 4> Previous{};
};

} // namespace tanks::view::win32
