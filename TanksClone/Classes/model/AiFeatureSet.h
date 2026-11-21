#pragma once

#include <array>
#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "model/ConfigNode.h"

namespace tanks::model
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
	Count
};

// Feature name -> on/off, as written under "ai_features" in an AI profile, enemy, difficulty mode or level.
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
	// Unknown names are skipped.
	void Apply(const AiSwitches& InSwitches);

	static std::optional<AiFeature> FromName(std::string_view InName);
	// The "ai_features" object of InNode; empty when it has none.
	static AiSwitches ReadSwitches(const IConfigNode& InNode);

private:
	std::array<bool, static_cast<size_t>(AiFeature::Count)> Enabled{};
};

} // namespace tanks::model
