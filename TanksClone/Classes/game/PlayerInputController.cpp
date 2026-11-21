#include "game/PlayerInputController.h"

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/CommandsManager.h"

namespace tanks
{

PlayerInputController::PlayerInputController()
	: Controls([](const CommandType InCommand) { GameContext::Get().GetManager<CommandsManager>().AddCommand(InCommand, PlayerName); })
{
	Listener = GameContext::Get().GetManager<InputManager>().AddListener([this](const model::InputEvent& InEvent) { Controls.OnAction(InEvent); });
}

PlayerInputController::~PlayerInputController()
{
	GameContext::Get().GetManager<InputManager>().RemoveListener(Listener);
}

} // namespace tanks
