#include "game/GameCommand.h"

namespace game
{

GameCommand::GameCommand(const CommandType InType, std::string_view InTarget, const cocos2d::ValueMap& InData)
	: Kind(InType)
	, Target(InTarget)
	, TargetType(ObjectType::Undefined)
	, Payload(InData)
{
}

GameCommand::GameCommand(const CommandType InType, const ObjectType InTarget, const cocos2d::ValueMap& InData)
	: Kind(InType)
	, TargetType(InTarget)
	, Payload(InData)
{
}

void GameCommand::SetTarget(std::string_view InTarget)
{
	Target = InTarget;
}

void GameCommand::SetTargetType(const ObjectType InTargetType)
{
	TargetType = InTargetType;
}

void GameCommand::SetData(const std::string& InKey, const cocos2d::Value& InValue)
{
	Payload[InKey] = InValue;
}

const cocos2d::ValueMap& GameCommand::GetData() const
{
	return Payload;
}

const std::string& GameCommand::GetTarget() const
{
	return Target;
}

ObjectType GameCommand::GetTargetType() const
{
	return TargetType;
}

CommandType GameCommand::GetType() const
{
	return Kind;
}

} // namespace game
