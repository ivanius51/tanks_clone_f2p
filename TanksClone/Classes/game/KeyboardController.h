#pragma once

#include "game/PlayerControls.h"

namespace tanks
{

/** Sends the player's key presses, received through the input port, to the player object as commands. */
class KeyboardController
{
public:
	KeyboardController();
	~KeyboardController();
	// Off while the game is paused: no key reaches the player, and the held keys are let go.
	void SetGameplayInputEnabled(bool bInEnabled);
	bool IsGameplayInputEnabled() const;
	KeyboardController(const KeyboardController&) = delete;
	KeyboardController& operator=(const KeyboardController&) = delete;

private:
	PlayerControls Controls;
};

} // namespace tanks
