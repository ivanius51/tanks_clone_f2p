#include "game/PlayerControls.h"

#include <algorithm>

namespace tanks
{

namespace
{
	// Returns true and fills OutCommand if the key is one of the movement keys.
	bool KeyToMoveCommand(const model::Key InKey, CommandType& OutCommand)
	{
		switch ( InKey )
		{
			case model::Key::Left:
			case model::Key::A:
				OutCommand = CommandType::MoveLeft;
				return true;
			case model::Key::Right:
			case model::Key::D:
				OutCommand = CommandType::MoveRight;
				return true;
			case model::Key::Up:
			case model::Key::W:
				OutCommand = CommandType::MoveForward;
				return true;
			case model::Key::Down:
			case model::Key::S:
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

void PlayerControls::OnKey(const model::KeyEvent& InEvent)
{
	if ( InEvent.bPressed )
	{
		OnPressed(InEvent.Code);
	}
	else
	{
		OnReleased(InEvent.Code);
	}
}

void PlayerControls::OnPressed(const model::Key InKey)
{
	CommandType Command = CommandType::Undefined;
	if ( KeyToMoveCommand(InKey, Command) )
	{
		HeldMoveKeys.erase(std::remove(HeldMoveKeys.begin(), HeldMoveKeys.end(), InKey), HeldMoveKeys.end());
		HeldMoveKeys.push_back(InKey);
		Sink(Command);
	}
	else if ( InKey == model::Key::Space )
	{
		if ( !bFireHeld )
		{
			bFireHeld = true;
			Sink(CommandType::StartFiring);
		}
	}
	else if ( InKey == model::Key::E )
	{
		Sink(CommandType::UseAbility);
	}
	else if ( InKey == model::Key::Q || InKey == model::Key::Tab )
	{
		Sink(CommandType::SwitchWeapon);
	}
}

void PlayerControls::ReleaseAll()
{
	if ( bFireHeld )
	{
		bFireHeld = false;
		Sink(CommandType::StopFiring);
	}
	if ( !HeldMoveKeys.empty() )
	{
		HeldMoveKeys.clear();
		Sink(CommandType::Stop);
	}
}

bool PlayerControls::IsFireHeld() const
{
	return bFireHeld;
}

bool PlayerControls::IsMoveHeld() const
{
	return !HeldMoveKeys.empty();
}

void PlayerControls::OnReleased(const model::Key InKey)
{
	if ( InKey == model::Key::Space )
	{
		if ( bFireHeld )
		{
			bFireHeld = false;
			Sink(CommandType::StopFiring);
		}
		return;
	}

	const auto HeldIt = std::find(HeldMoveKeys.begin(), HeldMoveKeys.end(), InKey);
	if ( HeldIt == HeldMoveKeys.end() )
	{
		return;
	}

	const bool bWasCurrent = (HeldIt == HeldMoveKeys.end() - 1);
	HeldMoveKeys.erase(HeldIt);
	if ( !bWasCurrent )
	{
		return; // the direction is still decided by a key that is held
	}

	CommandType Command = CommandType::Stop;
	if ( !HeldMoveKeys.empty() )
	{
		// Go back to the direction of the key that is still held.
		KeyToMoveCommand(HeldMoveKeys.back(), Command);
	}
	Sink(Command);
}

} // namespace tanks
