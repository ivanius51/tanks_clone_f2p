#include "GameCommand.h"
NS_CC_BEGIN

GameCommand::GameCommand(eCommandType aType, std::string aTarget, ValueMap aData)
	: mType(aType)
	, mTarget(aTarget)
	, mData(aData)
	, mTargetType(eObjectType::OBJECT_TYPE_UNDEFINED)
{
}

GameCommand::GameCommand(eCommandType aType, eObjectType aTarget, ValueMap aData)
	: mType(aType)
	, mData(aData)
	, mTargetType(aTarget)
{
}

void GameCommand::setTarget(const std::string& aTarget)
{
	mTarget = aTarget;
}

void GameCommand::setTargetType(eObjectType aTargetType)
{
	mTargetType = aTargetType;
}

void GameCommand::setData(const std::string& aKey, const Value& aValue)
{
	mData[aKey] = aValue;
}

const ValueMap& GameCommand::getData() const
{
	return mData;
}

const std::string& GameCommand::getTarget() const
{
	return mTarget;
}

const eObjectType& GameCommand::getTargetType() const
{
	return mTargetType;
}

const eCommandType& GameCommand::getType() const
{
	return mType;
}



NS_CC_END