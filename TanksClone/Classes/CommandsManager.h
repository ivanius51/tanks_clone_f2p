#ifndef _CommandsManager
#define _CommandsManager

#include "GameCommand.h"

NS_CC_BEGIN
class CommandsManager
{
private:
	CommandsManager();
	~CommandsManager();
	CommandsManager(const CommandsManager&);
	CommandsManager& operator=(const CommandsManager&);

	std::deque<GameCommand> mCommands;

public:
	static CommandsManager& getInstance();

	void addCommand(eCommandType aType = eCommandType::COMMAND_TYPE_UNDEFINED, std::string aTarget = "", ValueMap aData = ValueMap());
	void addCommand(eCommandType aType = eCommandType::COMMAND_TYPE_UNDEFINED, eObjectType aTarget = eObjectType::OBJECT_TYPE_UNDEFINED, ValueMap aData = ValueMap());
	void addCommand(const GameCommand& aCommand);

	void forceSend();
};

#define COMMAND CommandsManager::getInstance()

NS_CC_END
#endif
