#include "game/KeyboardController.h"

#include <algorithm>

#include "core/GameConstants.h"
#include "core/GameContext.h"
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
	KeyListener = cocos2d::EventListenerKeyboard::create();
	KeyListener->onKeyPressed = [this](const cocos2d::EventKeyboard::KeyCode InKeyCode, cocos2d::Event* const InEvent) { OnKeyPressed(InKeyCode, InEvent); };
	KeyListener->onKeyReleased = [this](const cocos2d::EventKeyboard::KeyCode InKeyCode, cocos2d::Event* const InEvent) { OnKeyReleased(InKeyCode, InEvent); };
	cocos2d::Director::getInstance()->getEventDispatcher()->addEventListenerWithFixedPriority(KeyListener, ListenerPriority);
}

KeyboardController::~KeyboardController()
{
	cocos2d::Director::getInstance()->getEventDispatcher()->removeEventListener(KeyListener);
}

void KeyboardController::OnKeyPressed(const cocos2d::EventKeyboard::KeyCode InKeyCode, cocos2d::Event* /*InEvent*/)
{
	auto& CurrentCommands = GameContext::Get().GetManager<CommandsManager>();

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

	auto& CurrentCommands = GameContext::Get().GetManager<CommandsManager>();
	CommandType CurrentCommand = CommandType::Stop;
	if ( !HeldMoveKeys.empty() )
	{
		// Go back to the direction of the key that is still held.
		KeyToMoveCommand(HeldMoveKeys.back(), CurrentCommand);
	}
	CurrentCommands.AddCommand(CurrentCommand, PlayerName);
}

} // namespace tanks
