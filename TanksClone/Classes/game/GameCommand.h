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
	explicit GameCommand(ECommandType InType = ECommandType::Undefined, std::string_view InTarget = "", const cocos2d::ValueMap& InData = cocos2d::ValueMap());
	GameCommand(ECommandType InType, EObjectType InTarget, const cocos2d::ValueMap& InData = cocos2d::ValueMap());

	void SetTarget(std::string_view InTarget);
	void SetTargetType(EObjectType InTargetType);
	void SetData(const std::string& InKey, const cocos2d::Value& InValue);

	const cocos2d::ValueMap& GetData() const;
	const std::string& GetTarget() const;
	EObjectType GetTargetType() const;
	ECommandType GetType() const;

private:
	ECommandType Kind;
	std::string Target;
	EObjectType TargetType;
	cocos2d::ValueMap Payload;
};
} // namespace game
