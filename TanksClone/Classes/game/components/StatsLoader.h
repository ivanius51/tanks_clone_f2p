#pragma once


#include "game/components/Stats.h"
#include "model/ConfigNode.h"
#include "templates/TContainers.h"

namespace game
{
/** The only place that knows the layout of stat modifiers in configs; game code gets plain StatModifier lists. */
class StatsLoader
{
public:
	static TArray<StatModifier> ReadModifiers(const model::IConfigNode& InNode);
	static bool ReadModifier(const model::IConfigNode& InNode, StatModifier& OutModifier);
};
} // namespace game
