#include "game/CommandsManager.h"

#include "core/GameContext.h"
#include "game/ObjectsFactory.h"

namespace game
{

namespace
{
	constexpr const char* SendScheduleKey = "SEND_COMMANDS";
}

CommandsManager::CommandsManager()
{
	// interval 0: every frame
	cocos2d::Director::getInstance()->getScheduler()->schedule(
		[this](float) { ForceSend(); }, this,
		0.0f, CC_REPEAT_FOREVER, 0.0f, false, SendScheduleKey
	);
}

CommandsManager::~CommandsManager()
{
	cocos2d::Director::getInstance()->getScheduler()->unschedule(SendScheduleKey, this);
}

void CommandsManager::AddCommand(const CommandType InType, std::string_view InTarget, const cocos2d::ValueMap& InData)
{
	Commands.emplace_back(InType, InTarget, InData);
}

void CommandsManager::AddCommand(const CommandType InType, const ObjectType InTarget, const cocos2d::ValueMap& InData)
{
	Commands.emplace_back(InType, InTarget, InData);
}

void CommandsManager::AddCommand(const GameCommand& InCommand)
{
	Commands.push_back(InCommand);
}

void CommandsManager::ForceSend()
{
	// Handlers may queue new commands: those are delivered on the next frame.
	std::deque<GameCommand> CurrentPending;
	CurrentPending.swap(Commands);

	for ( const auto& CurrentCommand : CurrentPending )
	{
		Deliver(CurrentCommand);
	}
}

void CommandsManager::Deliver(const GameCommand& InCommand) const
{
	auto& Factory = GameContext::Get<ObjectsFactory>();

	if ( !InCommand.GetTarget().empty() )
	{
		GameObject* Receiver = Factory.FindObject(InCommand.GetTarget(), InCommand.GetTargetType());
		if ( Receiver )
		{
			Receiver->OnCommandReceived(InCommand);
		}
	}
	else if ( InCommand.GetTargetType() != ObjectType::Undefined )
	{
		// No name: broadcast to every object of the type.
		Factory.ForEachObject(InCommand.GetTargetType(),
			[&InCommand](GameObject& InObject) { InObject.OnCommandReceived(InCommand); });
	}
}

} // namespace game
