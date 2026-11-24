#pragma once

#include <deque>
#include <string_view>

#include "game/GameCommand.h"

namespace tanks
{
/** Queues commands and delivers them to their targets once per frame. */
class CommandsManager
{
public:
	CommandsManager();
	~CommandsManager();
	CommandsManager(const CommandsManager&) = delete;
	CommandsManager& operator=(const CommandsManager&) = delete;

	void AddCommand(CommandType InType = CommandType::Undefined, std::string_view InTarget = "", const cocos2d::ValueMap& InData = cocos2d::ValueMap());
	void AddCommand(CommandType InType, ObjectType InTarget, const cocos2d::ValueMap& InData = cocos2d::ValueMap());
	void AddCommand(const GameCommand& InCommand);

	void ForceSend();

private:
	void Deliver(const GameCommand& InCommand) const;

	std::deque<GameCommand> Commands;
};
} // namespace tanks
