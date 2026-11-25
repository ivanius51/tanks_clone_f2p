#include "game/PlayerControls.h"

#include <algorithm>

namespace game
{

namespace
{
	bool ToMoveCommand(const model::InputAction InAction, CommandType& OutCommand)
	{
		switch ( InAction )
		{
			case model::InputAction::MoveLeft:
				OutCommand = CommandType::MoveLeft;
				return true;
			case model::InputAction::MoveRight:
				OutCommand = CommandType::MoveRight;
				return true;
			case model::InputAction::MoveUp:
				OutCommand = CommandType::MoveForward;
				return true;
			case model::InputAction::MoveDown:
				OutCommand = CommandType::MoveBack;
				return true;
			default:
				return false;
		}
	}
}

PlayerControls::PlayerControls(const CommandSink& InSink) : Sink(InSink)
{
}

void PlayerControls::OnAction(const model::InputEvent& InEvent)
{
	if ( InEvent.bPressed )
	{
		OnPressed(InEvent.Action);
	}
	else
	{
		OnReleased(InEvent.Action);
	}
}

void PlayerControls::OnPressed(const model::InputAction InAction)
{
	CommandType Command = CommandType::Undefined;
	if ( ToMoveCommand(InAction, Command) )
	{
		HeldMoves.erase(std::remove(HeldMoves.begin(), HeldMoves.end(), InAction), HeldMoves.end());
		HeldMoves.push_back(InAction);
		Sink(Command);
	}
	else if ( InAction == model::InputAction::Fire && !bFireHeld )
	{
		bFireHeld = true;
		Sink(CommandType::StartFiring);
	}
	else if ( InAction == model::InputAction::Ability1 )
	{
		Sink(CommandType::UseAbility);
	}
	else if ( InAction == model::InputAction::SwitchWeapon )
	{
		Sink(CommandType::SwitchWeapon);
	}
}

void PlayerControls::OnReleased(const model::InputAction InAction)
{
	if ( InAction == model::InputAction::Fire )
	{
		if ( bFireHeld )
		{
			bFireHeld = false;
			Sink(CommandType::StopFiring);
		}
		return;
	}

	const auto HeldIt = std::find(HeldMoves.begin(), HeldMoves.end(), InAction);
	if ( HeldIt == HeldMoves.end() )
	{
		return;
	}
	const bool bWasCurrent = HeldIt == HeldMoves.end() - 1;
	HeldMoves.erase(HeldIt);
	if ( !bWasCurrent )
	{
		return;
	}

	CommandType Command = CommandType::Stop;
	if ( !HeldMoves.empty() )
	{
		ToMoveCommand(HeldMoves.back(), Command);
	}
	Sink(Command);
}

void PlayerControls::ReleaseAll()
{
	if ( bFireHeld )
	{
		bFireHeld = false;
		Sink(CommandType::StopFiring);
	}
	if ( !HeldMoves.empty() )
	{
		HeldMoves.clear();
		Sink(CommandType::Stop);
	}
}

bool PlayerControls::IsFireHeld() const
{
	return bFireHeld;
}

bool PlayerControls::IsMoveHeld() const
{
	return !HeldMoves.empty();
}

} // namespace game
