#include "GameCommand.h"
NS_CC_BEGIN

GameCommand::GameCommand(eCommandType aType, std::string aTarget, ValueMap aData)
{
}

const ValueMap& GameCommand::getData() const
{
	return mData;
}

const std::string& GameCommand::getTarget() const
{
	return mTarget;
}

const eCommandType& GameCommand::getType() const
{
	return mType;
}



NS_CC_END