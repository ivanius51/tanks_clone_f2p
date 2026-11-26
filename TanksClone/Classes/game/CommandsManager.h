#pragma once

#include <deque>
#include <string_view>

#include "game/GameCommand.h"

namespace game
{
/** Queues commands and delivers them to their targets once per frame. */
class CommandsManager
{
public:
	CommandsManager();
	~CommandsManager();
	CommandsManager(const CommandsManager&) = delete;
	CommandsManager& operator=(const CommandsManager&) = delete;

	void AddCommand(ECommandType InType = ECommandType::Undefined, std::string_view InTarget = "", const model::JsonValue::Object& InData = model::JsonValue::Object());
	void AddCommand(ECommandType InType, EObjectType InTarget, const model::JsonValue::Object& InData = model::JsonValue::Object());
	void AddCommand(const GameCommand& InCommand);

	void ForceSend();

private:
	void Deliver(const GameCommand& InCommand) const;

	std::deque<GameCommand> Commands;
};
} // namespace game
