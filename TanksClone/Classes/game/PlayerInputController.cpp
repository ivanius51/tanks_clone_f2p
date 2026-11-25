#include "game/PlayerInputController.h"

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/CommandsManager.h"

namespace game
{

PlayerInputController::PlayerInputController()
	: Controls([](const ECommandType InCommand) { GameContext::Get<CommandsManager>().AddCommand(InCommand, PlayerName); })
{
	Listener = GameContext::Get<InputManager>().AddListener([this](const model::InputEvent& InEvent) { Controls.OnAction(InEvent); });
}

PlayerInputController::~PlayerInputController()
{
	GameContext::Get<InputManager>().RemoveListener(Listener);
}

} // namespace game
