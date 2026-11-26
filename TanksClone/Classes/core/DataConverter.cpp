#include "core/DataConverter.h"

namespace game
{

cocos2d::Vec2 DataConverter::ValueToVec2(const model::JsonValue& InValue)
{
	cocos2d::Vec2 Result;
	if ( InValue.IsArray() )
	{
		const model::JsonValue::Array& ValueVec = InValue.GetArray();
		if ( ValueVec.Size() > 1 )
		{
			Result.x = ValueVec[0].AsFloat();
			Result.y = ValueVec[1].AsFloat();
		}
	}
	return Result;
}

cocos2d::Color3B DataConverter::ValueToColor3B(const model::JsonValue& InValue)
{
	cocos2d::Color3B Result;
	if ( InValue.IsArray() )
	{
		const model::JsonValue::Array& ValueVec = InValue.GetArray();
		if ( ValueVec.Size() > 2 )
		{
			Result.r = static_cast<GLubyte>(ValueVec[0].AsInt());
			Result.g = static_cast<GLubyte>(ValueVec[1].AsInt());
			Result.b = static_cast<GLubyte>(ValueVec[2].AsInt());
		}
	}
	return Result;
}

int DataConverter::GetInt(const model::JsonValue::Object& InMap, const std::string_view InKey, const int InFallback)
{
	const model::JsonValue* const Found = InMap.Find(InKey);
	return Found ? Found->AsInt(InFallback) : InFallback;
}

float DataConverter::GetFloat(const model::JsonValue::Object& InMap, const std::string_view InKey, const float InFallback)
{
	const model::JsonValue* const Found = InMap.Find(InKey);
	return Found ? Found->AsFloat(InFallback) : InFallback;
}

bool DataConverter::GetBool(const model::JsonValue::Object& InMap, const std::string_view InKey, const bool bInFallback)
{
	const model::JsonValue* const Found = InMap.Find(InKey);
	return Found ? Found->AsBool(bInFallback) : bInFallback;
}

std::string DataConverter::GetString(const model::JsonValue::Object& InMap, const std::string_view InKey, const std::string_view InFallback)
{
	const model::JsonValue* const Found = InMap.Find(InKey);
	return Found ? Found->AsString(InFallback) : std::string(InFallback);
}

} // namespace game
