#pragma once

#include <vector>
#include "game/components/Stats.h"
#include "model/ConfigNode.h"

namespace tanks
{

/** The only place that knows the layout of stat modifiers in configs; game code gets plain StatModifier lists. */
class StatsLoader
{
public:
	// An array of {"stat": "Speed", "add": 10, "mul": 1.5}. Entries with an unknown stat are logged and skipped.
	static std::vector<StatModifier> ReadModifiers(const model::IConfigNode& InNode);
	// False when "stat" is missing or unknown; "add" defaults to 0, "mul" to 1.
	static bool ReadModifier(const model::IConfigNode& InNode, StatModifier& OutModifier);
};

} // namespace tanks
