#pragma once

#include <functional>
#include <vector>

#include "core/ObjectEnums.h"
#include "model/InputTypes.h"

namespace tanks
{

/**
 * Turns key events into player commands: which keys move, fire, use the ability or switch the weapon.
 * It remembers the movement keys that are held, so releasing the newest one goes back to the one still held,
 * and whether the fire key is held, so the tank keeps firing until it is released.
 * Knows nothing about the engine: input arrives as model::KeyEvent and commands leave through a function.
 */
class PlayerControls
{
public:
	using CommandSink = std::function<void(CommandType)>;

	explicit PlayerControls(const CommandSink& InSink);

	void OnKey(const model::KeyEvent& InEvent);
	// Forgets every held key and stops the tank, e.g. when the game pauses and releases are no longer seen.
	void ReleaseAll();

	bool IsFireHeld() const;
	bool IsMoveHeld() const;

private:
	void OnPressed(model::Key InKey);
	void OnReleased(model::Key InKey);

	CommandSink Sink;
	// Movement keys currently held, in the order they were pressed; the last one decides the direction.
	std::vector<model::Key> HeldMoveKeys;
	bool bFireHeld = false;
};

} // namespace tanks
