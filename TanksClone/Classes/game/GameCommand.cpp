#include "game/GameCommand.h"

namespace game
{

GameCommand::GameCommand(const ECommandType InType, std::string_view InTarget, const model::JsonValue::Object& InData)
	: Kind(InType)
	, Target(InTarget)
	, TargetType(EObjectType::Undefined)
	, Payload(InData)
{
}

GameCommand::GameCommand(const ECommandType InType, const EObjectType InTarget, const model::JsonValue::Object& InData)
	: Kind(InType)
	, TargetType(InTarget)
	, Payload(InData)
{
}

void GameCommand::SetTarget(std::string_view InTarget)
{
	Target = InTarget;
}

void GameCommand::SetTargetType(const EObjectType InTargetType)
{
	TargetType = InTargetType;
}

void GameCommand::SetData(const std::string_view InKey, const model::JsonValue& InValue)
{
	Payload[InKey] = InValue;
}

const model::JsonValue::Object& GameCommand::GetData() const
{
	return Payload;
}

const std::string& GameCommand::GetTarget() const
{
	return Target;
}

EObjectType GameCommand::GetTargetType() const
{
	return TargetType;
}

ECommandType GameCommand::GetType() const
{
	return Kind;
}

} // namespace game
