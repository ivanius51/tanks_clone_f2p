#include "CommandsManager.h"
#include "ObjectFactory.h"


NS_CC_BEGIN
CommandsManager::CommandsManager()
{
	Director::getInstance()->getScheduler()->schedule(
		CC_CALLBACK_0(CommandsManager::forceSend, this), this,
		0.02f, CC_REPEAT_FOREVER, 0.0f, false, "SEND_COMMANDS"
	);
}

CommandsManager::~CommandsManager()
{
}

CommandsManager& CommandsManager::getInstance()
{
	static CommandsManager instance;
	return instance;
}

void CommandsManager::addCommand(eCommandType aType, std::string aTarget, ValueMap aData)
{
	mCommands.push_back(GameCommand(aType, aTarget, aData));
}

void CommandsManager::addCommand(eCommandType aType, eObjectType aTarget, ValueMap aData)
{
	mCommands.push_back(GameCommand(aType, aTarget, aData));
}

void CommandsManager::addCommand(const GameCommand& aCommand)
{
	mCommands.push_back(aCommand);
}

void CommandsManager::forceSend()
{
	if ( !mCommands.empty() )
	{
		auto currentCommand = mCommands.front();
		mCommands.pop_front();
		std::string target = currentCommand.getTarget();
		eObjectType targetType = currentCommand.getTargetType();

		if ( targetType != eObjectType::OBJECT_TYPE_UNDEFINED )
		{
			auto objects = *FACTORY.getObjectsByType(targetType);
			if ( !target.empty() )
			{
				for ( auto& objectIt : objects )
				{
					if ( objectIt->getName() == target )
					{
						objectIt->onCommandReceived(currentCommand);
						break;
					}
				}
			}
		}
		else
		{
			auto objects = FACTORY.getObjects();
			if ( !target.empty() )
			{
				bool exitLoop = false;
				for ( auto& objectsIt : objects )
				{
					for ( auto& objectIt : objectsIt.second )
					{
						if ( objectIt->getName() == target )
						{
							objectIt->onCommandReceived(currentCommand);
							exitLoop = true; 
							break;
						}
					}
					if ( exitLoop )
					{
						break;
					}
				}
			}
		}
	}
}

NS_CC_END