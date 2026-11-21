#pragma once

#include <functional>
#include <vector>

#include "core/ObjectEnums.h"
#include "model/InputTypes.h"

namespace tanks
{

/**
 * Turns key events into player commands: which keys move, shoot, use the ability or switch the weapon.
 * It remembers the movement keys that are held, so releasing the newest one goes back to the one still held.
 * Knows nothing about the engine: input arrives as model::KeyEvent and commands leave through a function.
 */
class PlayerControls
{
public:
	using CommandSink = std::function<void(CommandType)>;

	explicit PlayerControls(const CommandSink& InSink);

	void OnKey(const model::KeyEvent& InEvent);

private:
	void OnPressed(model::Key InKey);
	void OnReleased(model::Key InKey);

	CommandSink Sink;
	// Movement keys currently held, in the order they were pressed; the last one decides the direction.
	std::vector<model::Key> HeldMoveKeys;
};

} // namespace tanks
