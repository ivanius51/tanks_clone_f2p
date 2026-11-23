#pragma once

#include <array>
#include <string>
#include <string_view>

#include "model/ConfigNode.h"
#include "model/JsonValue.h"

namespace tanks::model
{

/**
 * Turns the JSON tree of a level into file text with a stable layout, so saving a level changes only what was edited:
 * tabs, level keys in a fixed order (then unknown keys sorted), one map row per line, short objects on one line.
 */
class LevelJsonWriter
{
public:
	// Top-level keys of a level file in the order they are written.
	static const std::array<std::string_view, 10>& GetKnownKeys();

	static bool IsKnownKey(std::string_view InKey);

	static std::string Write(const JsonValue& InRoot);

	// A float as the shortest decimal that reads back to the same float.
	static JsonValue FromFloat(float InValue);

	// Copies a config subtree. An empty object and an empty array are not told apart by IConfigNode and come out as an empty object.
	static JsonValue FromNode(const IConfigNode& InNode);
};

} // namespace tanks::model
