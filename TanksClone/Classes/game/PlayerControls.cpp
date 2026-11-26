#include "game/PlayerControls.h"

#include <algorithm>

namespace game
{

namespace
{
	bool ToMoveCommand(const model::InputAction InAction, ECommandType& OutCommand)
	{
		switch ( InAction )
		{
			case model::InputAction::MoveLeft:
				OutCommand = ECommandType::MoveLeft;
				return true;
			case model::InputAction::MoveRight:
				OutCommand = ECommandType::MoveRight;
				return true;
			case model::InputAction::MoveUp:
				OutCommand = ECommandType::MoveForward;
				return true;
			case model::InputAction::MoveDown:
				OutCommand = ECommandType::MoveBack;
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
	ECommandType Command = ECommandType::Undefined;
	if ( ToMoveCommand(InAction, Command) )
	{
		HeldMoves.Erase(std::remove(HeldMoves.begin(), HeldMoves.end(), InAction), HeldMoves.end());
		HeldMoves.PushBack(InAction);
		Sink(Command);
	}
	else if ( InAction == model::InputAction::Fire && !bFireHeld )
	{
		bFireHeld = true;
		Sink(ECommandType::StartFiring);
	}
	else if ( InAction == model::InputAction::Ability1 )
	{
		Sink(ECommandType::UseAbility);
	}
	else if ( InAction == model::InputAction::SwitchWeapon )
	{
		Sink(ECommandType::SwitchWeapon);
	}
}

void PlayerControls::OnReleased(const model::InputAction InAction)
{
	if ( InAction == model::InputAction::Fire )
	{
		if ( bFireHeld )
		{
			bFireHeld = false;
			Sink(ECommandType::StopFiring);
		}
		return;
	}

	const auto HeldIt = std::find(HeldMoves.begin(), HeldMoves.end(), InAction);
	if ( HeldIt == HeldMoves.end() )
	{
		return;
	}
	const bool bWasCurrent = HeldIt == HeldMoves.end() - 1;
	HeldMoves.Erase(HeldIt);
	if ( !bWasCurrent )
	{
		return;
	}

	ECommandType Command = ECommandType::Stop;
	if ( !HeldMoves.IsEmpty() )
	{
		ToMoveCommand(HeldMoves.Back(), Command);
	}
	Sink(Command);
}

void PlayerControls::ReleaseAll()
{
	if ( bFireHeld )
	{
		bFireHeld = false;
		Sink(ECommandType::StopFiring);
	}
	if ( !HeldMoves.IsEmpty() )
	{
		HeldMoves.Clear();
		Sink(ECommandType::Stop);
	}
}

bool PlayerControls::IsFireHeld() const
{
	return bFireHeld;
}

bool PlayerControls::IsMoveHeld() const
{
	return !HeldMoves.IsEmpty();
}

} // namespace game
