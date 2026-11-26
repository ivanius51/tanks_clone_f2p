#pragma once

#include <string>
#include <string_view>

#include "core/ObjectEnums.h"

namespace game
{
/** A message addressed to an object by name, or to all objects of a given type. */
class GameCommand
{
public:
	explicit GameCommand(ECommandType InType = ECommandType::Undefined, std::string_view InTarget = "", const model::JsonValue::Object& InData = model::JsonValue::Object());
	GameCommand(ECommandType InType, EObjectType InTarget, const model::JsonValue::Object& InData = model::JsonValue::Object());

	void SetTarget(std::string_view InTarget);
	void SetTargetType(EObjectType InTargetType);
	void SetData(std::string_view InKey, const model::JsonValue& InValue);

	const model::JsonValue::Object& GetData() const;
	const std::string& GetTarget() const;
	EObjectType GetTargetType() const;
	ECommandType GetType() const;

private:
	ECommandType Kind;
	std::string Target;
	EObjectType TargetType;
	model::JsonValue::Object Payload;
};
} // namespace game
