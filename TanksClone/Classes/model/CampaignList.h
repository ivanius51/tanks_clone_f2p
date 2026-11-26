#pragma once

#include <string>

#include "model/ConfigNode.h"
#include "templates/TContainers.h"

namespace game::model
{
/** The shipped campaign: the level config ids in play order, read from `configs/levels/campaign.json`. */
class CampaignList
{
public:
	static CampaignList FromConfig(const IConfigNode& InNode);

	const TArray<std::string>& GetLevelIds() const { return LevelIds; }
	bool IsEmpty() const { return LevelIds.IsEmpty(); }

private:
	TArray<std::string> LevelIds;
};
} // namespace game::model
