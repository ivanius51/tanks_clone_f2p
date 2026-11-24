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
	static std::vector<StatModifier> ReadModifiers(const model::IConfigNode& InNode);
	static bool ReadModifier(const model::IConfigNode& InNode, StatModifier& OutModifier);
};
} // namespace tanks
