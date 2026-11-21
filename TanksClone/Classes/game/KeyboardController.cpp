#include "game/KeyboardController.h"

#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/CommandsManager.h"
#include "ports/IInputSource.h"

namespace tanks
{

KeyboardController::KeyboardController()
	: Controls([](const CommandType InCommand) { GameContext::Get().GetManager<CommandsManager>().AddCommand(InCommand, PlayerName); })
{
	GameContext::Get().GetInputSource().SetKeyHandler([this](const model::KeyEvent& InEvent) { Controls.OnKey(InEvent); });
}

KeyboardController::~KeyboardController()
{
	GameContext::Get().GetInputSource().SetKeyHandler(nullptr);
}

void KeyboardController::SetGameplayInputEnabled(const bool bInEnabled)
{
	Controls.SetEnabled(bInEnabled);
}

bool KeyboardController::IsGameplayInputEnabled() const
{
	return Controls.IsEnabled();
}

} // namespace tanks
