#pragma once

#include <array>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "model/ConfigNode.h"

namespace game::model
{
enum class AiFeature
{
	LeadShots,
	Dodge,
	Flank,
	Retreat,
	GuardDrops,
	Group,
	AttackBase,
	AntiCamp,
	Cover,
	FakeTurn,
	Count
};

using AiSwitches = std::map<std::string, bool>;

/**
 * Which smarter AI behaviours an enemy uses. The AI profile sets them first, then the enemy's own config,
 * the difficulty mode and the level switch single features on or off, the later one winning.
 */
class AiFeatureSet
{
public:
	bool Has(AiFeature InFeature) const;
	void Set(AiFeature InFeature, bool bInEnabled);
	void Apply(const AiSwitches& InSwitches);

	static std::optional<AiFeature> FromName(std::string_view InName);
	static AiSwitches ReadSwitches(const IConfigNode& InNode);

private:
	std::array<bool, static_cast<size_t>(AiFeature::Count)> Enabled{};
};
} // namespace game::model
