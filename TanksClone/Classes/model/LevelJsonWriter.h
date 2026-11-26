#pragma once

#include <array>
#include <string>
#include <string_view>

#include "model/ConfigNode.h"
#include "model/JsonValue.h"

namespace game::model
{
/**
 * Turns the JSON tree of a level into file text with a stable layout, so saving a level changes only what was edited:
 * tabs, level keys in a fixed order (then unknown keys sorted), one map row per line, short objects on one line.
 */
class LevelJsonWriter
{
public:
	static const std::array<std::string_view, 12>& GetKnownKeys();

	static bool IsKnownKey(std::string_view InKey);

	static std::string Write(const JsonValue& InRoot);

	static JsonValue FromFloat(float InValue);

	static JsonValue FromNode(const IConfigNode& InNode);
};
} // namespace game::model
