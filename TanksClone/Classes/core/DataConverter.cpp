#include "core/DataConverter.h"

namespace game
{

cocos2d::Vec2 DataConverter::ValueToVec2(const cocos2d::Value& InValue)
{
	cocos2d::Vec2 Result;
	if ( InValue.getType() == cocos2d::Value::Type::VECTOR )
	{
		const cocos2d::ValueVector& ValueVec = InValue.asValueVector();
		if ( ValueVec.size() > 1 )
		{
			Result.x = ValueVec[0].asFloat();
			Result.y = ValueVec[1].asFloat();
		}
	}
	return Result;
}

cocos2d::Color3B DataConverter::ValueToColor3B(const cocos2d::Value& InValue)
{
	cocos2d::Color3B Result;
	if ( InValue.getType() == cocos2d::Value::Type::VECTOR )
	{
		const cocos2d::ValueVector& ValueVec = InValue.asValueVector();
		if ( ValueVec.size() > 2 )
		{
			Result.r = static_cast<GLubyte>(ValueVec[0].asInt());
			Result.g = static_cast<GLubyte>(ValueVec[1].asInt());
			Result.b = static_cast<GLubyte>(ValueVec[2].asInt());
		}
	}
	return Result;
}

int DataConverter::GetInt(const cocos2d::ValueMap& InMap, const std::string& InKey, const int InFallback)
{
	auto FindIt = InMap.find(InKey);
	return FindIt != InMap.end() ? FindIt->second.asInt() : InFallback;
}

float DataConverter::GetFloat(const cocos2d::ValueMap& InMap, const std::string& InKey, const float InFallback)
{
	auto FindIt = InMap.find(InKey);
	return FindIt != InMap.end() ? FindIt->second.asFloat() : InFallback;
}

bool DataConverter::GetBool(const cocos2d::ValueMap& InMap, const std::string& InKey, const bool bInFallback)
{
	auto FindIt = InMap.find(InKey);
	return FindIt != InMap.end() ? FindIt->second.asBool() : bInFallback;
}

std::string DataConverter::GetString(const cocos2d::ValueMap& InMap, const std::string& InKey, const std::string& InFallback)
{
	auto FindIt = InMap.find(InKey);
	return FindIt != InMap.end() ? FindIt->second.asString() : InFallback;
}

} // namespace game
