#pragma once

#include <string>
#include <string_view>

#include "cocos2d.h"
#include "model/JsonValue.h"

namespace game
{
/**
 * Helpers for turning loosely typed config Values into engine types.
 * Malformed input gives a zero-initialized result.
 */
class DataConverter
{
public:
	static cocos2d::Vec2 ValueToVec2(const model::JsonValue& InValue);
	static cocos2d::Color3B ValueToColor3B(const model::JsonValue& InValue);

	static int GetInt(const model::JsonValue::Object& InMap, std::string_view InKey, int InFallback);
	static float GetFloat(const model::JsonValue::Object& InMap, std::string_view InKey, float InFallback);
	static bool GetBool(const model::JsonValue::Object& InMap, std::string_view InKey, bool bInFallback);
	static std::string GetString(const model::JsonValue::Object& InMap, std::string_view InKey, std::string_view InFallback);
};
} // namespace game
