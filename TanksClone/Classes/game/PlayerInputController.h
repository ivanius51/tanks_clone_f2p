#pragma once

#include "game/PlayerControls.h"
#include "services/InputManager.h"

namespace tanks
{

/** Sends the player's gameplay actions, received from the InputManager, to the player object as commands. */
class PlayerInputController
{
public:
	PlayerInputController();
	~PlayerInputController();
	PlayerInputController(const PlayerInputController&) = delete;
	PlayerInputController& operator=(const PlayerInputController&) = delete;

private:
	PlayerControls Controls;
	InputManager::ListenerId Listener = 0;
};

} // namespace tanks
