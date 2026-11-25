#pragma once

#include <string>
#include <string_view>

#include "cocos2d.h"

#include "core/ObjectEnums.h"

namespace game
{
/** A message addressed to an object by name, or to all objects of a given type. */
class GameCommand
{
public:
	explicit GameCommand(CommandType InType = CommandType::Undefined, std::string_view InTarget = "", const cocos2d::ValueMap& InData = cocos2d::ValueMap());
	GameCommand(CommandType InType, ObjectType InTarget, const cocos2d::ValueMap& InData = cocos2d::ValueMap());

	void SetTarget(std::string_view InTarget);
	void SetTargetType(ObjectType InTargetType);
	void SetData(const std::string& InKey, const cocos2d::Value& InValue);

	const cocos2d::ValueMap& GetData() const;
	const std::string& GetTarget() const;
	ObjectType GetTargetType() const;
	CommandType GetType() const;

private:
	CommandType Kind;
	std::string Target;
	ObjectType TargetType;
	cocos2d::ValueMap Payload;
};
} // namespace game
