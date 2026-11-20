#include "game/KeyboardController.h"

#include <algorithm>

#include "core/GameConstants.h"
#include "game/CommandsManager.h"

namespace tanks
{

namespace
{
	constexpr int ListenerPriority = -1;

	// Returns true and fills InCommand if the key is one of the movement keys.
	bool KeyToMoveCommand(const cocos2d::EventKeyboard::KeyCode InKeyCode, CommandType& InCommand)
	{
		switch ( InKeyCode )
		{
			case cocos2d::EventKeyboard::KeyCode::KEY_LEFT_ARROW:
			case cocos2d::EventKeyboard::KeyCode::KEY_A:
			case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_A:
				InCommand = CommandType::MoveLeft;
				return true;
			case cocos2d::EventKeyboard::KeyCode::KEY_RIGHT_ARROW:
			case cocos2d::EventKeyboard::KeyCode::KEY_D:
			case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_D:
				InCommand = CommandType::MoveRight;
				return true;
			case cocos2d::EventKeyboard::KeyCode::KEY_UP_ARROW:
			case cocos2d::EventKeyboard::KeyCode::KEY_W:
			case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_W:
				InCommand = CommandType::MoveForward;
				return true;
			case cocos2d::EventKeyboard::KeyCode::KEY_DOWN_ARROW:
			case cocos2d::EventKeyboard::KeyCode::KEY_S:
			case cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_S:
				InCommand = CommandType::MoveBack;
				return true;
			default:
				return false;
		}
	}
}

KeyboardController::KeyboardController()
{
	// The event dispatcher owns the listener and releases it together with the Director.
	auto CurrentListener = cocos2d::EventListenerKeyboard::create();
	CurrentListener->onKeyPressed = [this](const cocos2d::EventKeyboard::KeyCode InKeyCode, cocos2d::Event* const InEvent) { OnKeyPressed(InKeyCode, InEvent); };
	CurrentListener->onKeyReleased = [this](const cocos2d::EventKeyboard::KeyCode InKeyCode, cocos2d::Event* const InEvent) { OnKeyReleased(InKeyCode, InEvent); };
	cocos2d::Director::getInstance()->getEventDispatcher()->addEventListenerWithFixedPriority(CurrentListener, ListenerPriority);
}

KeyboardController& KeyboardController::GetInstance()
{
	static KeyboardController Instance;
	return Instance;
}

void KeyboardController::OnKeyPressed(const cocos2d::EventKeyboard::KeyCode InKeyCode, cocos2d::Event* /*InEvent*/)
{
	auto& CurrentCommands = CommandsManager::GetInstance();

	CommandType CurrentCommand = CommandType::Undefined;
	if ( KeyToMoveCommand(InKeyCode, CurrentCommand) )
	{
		HeldMoveKeys.erase(std::remove(HeldMoveKeys.begin(), HeldMoveKeys.end(), InKeyCode), HeldMoveKeys.end());
		HeldMoveKeys.push_back(InKeyCode);
		CurrentCommands.AddCommand(CurrentCommand, PlayerName);
	}
	else if ( InKeyCode == cocos2d::EventKeyboard::KeyCode::KEY_SPACE )
	{
		CurrentCommands.AddCommand(CommandType::Shoot, PlayerName);
	}
	else if ( InKeyCode == cocos2d::EventKeyboard::KeyCode::KEY_E || InKeyCode == cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_E )
	{
		CurrentCommands.AddCommand(CommandType::UseAbility, PlayerName);
	}
	else if ( InKeyCode == cocos2d::EventKeyboard::KeyCode::KEY_Q || InKeyCode == cocos2d::EventKeyboard::KeyCode::KEY_CAPITAL_Q
		|| InKeyCode == cocos2d::EventKeyboard::KeyCode::KEY_TAB )
	{
		CurrentCommands.AddCommand(CommandType::SwitchWeapon, PlayerName);
	}
}

void KeyboardController::OnKeyReleased(const cocos2d::EventKeyboard::KeyCode InKeyCode, cocos2d::Event* /*InEvent*/)
{
	auto HeldIt = std::find(HeldMoveKeys.begin(), HeldMoveKeys.end(), InKeyCode);
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

	auto& CurrentCommands = CommandsManager::GetInstance();
	CommandType CurrentCommand = CommandType::Stop;
	if ( !HeldMoveKeys.empty() )
	{
		// Go back to the direction of the key that is still held.
		KeyToMoveCommand(HeldMoveKeys.back(), CurrentCommand);
	}
	CurrentCommands.AddCommand(CurrentCommand, PlayerName);
}

} // namespace tanks
