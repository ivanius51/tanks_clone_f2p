#pragma once

#include <string>
#include <vector>

#include "model/ConfigNode.h"

namespace tanks::model
{

/** The shipped campaign: the level config ids in play order, read from `configs/levels/campaign.json`. */
class CampaignList
{
public:
	// Reads "levels": ["cfg.level.1", ...]. Empty and repeated ids are dropped, the order is kept.
	static CampaignList FromConfig(const IConfigNode& InNode);

	const std::vector<std::string>& GetLevelIds() const { return LevelIds; }
	bool IsEmpty() const { return LevelIds.empty(); }

private:
	std::vector<std::string> LevelIds;
};

} // namespace tanks::model
