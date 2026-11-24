#pragma once

#include <functional>
#include <vector>

#include "core/ObjectEnums.h"
#include "model/InputBindings.h"

namespace tanks
{
/**
 * Turns gameplay input actions into player commands. It remembers the movement actions that are held, so releasing
 * the newest one goes back to the one still held, and keeps firing while Fire is held.
 * Knows nothing about keys or the engine: actions arrive as model::InputEvent and commands leave through a function.
 */
class PlayerControls
{
public:
	using CommandSink = std::function<void(CommandType)>;

	explicit PlayerControls(const CommandSink& InSink);

	void OnAction(const model::InputEvent& InEvent);
	void ReleaseAll();

	bool IsFireHeld() const;
	bool IsMoveHeld() const;

private:
	void OnPressed(model::InputAction InAction);
	void OnReleased(model::InputAction InAction);

	CommandSink Sink;
	std::vector<model::InputAction> HeldMoves;
	bool bFireHeld = false;
};
} // namespace tanks
