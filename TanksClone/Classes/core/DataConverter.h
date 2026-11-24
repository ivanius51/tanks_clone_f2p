#pragma once

#include <string>

#include "cocos2d.h"

namespace tanks
{
/**
 * Helpers for turning loosely typed config Values into engine types.
 * Malformed input gives a zero-initialized result.
 */
class DataConverter
{
public:
	static cocos2d::Vec2 ValueToVec2(const cocos2d::Value& InValue);
	static cocos2d::Color3B ValueToColor3B(const cocos2d::Value& InValue);

	static int GetInt(const cocos2d::ValueMap& InMap, const std::string& InKey, int InFallback);
	static float GetFloat(const cocos2d::ValueMap& InMap, const std::string& InKey, float InFallback);
	static bool GetBool(const cocos2d::ValueMap& InMap, const std::string& InKey, bool bInFallback);
	static std::string GetString(const cocos2d::ValueMap& InMap, const std::string& InKey, const std::string& InFallback);
};
} // namespace tanks
