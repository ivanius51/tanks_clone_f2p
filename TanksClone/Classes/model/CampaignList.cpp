#include "model/CampaignList.h"

#include <algorithm>

namespace tanks::model
{

CampaignList CampaignList::FromConfig(const IConfigNode& InNode)
{
	CampaignList Result;
	const IConfigNode* const Levels = InNode.FindChild("levels");
	if ( !Levels )
	{
		return Result;
	}
	for ( std::size_t Index = 0; Index < Levels->GetItemCount(); ++Index )
	{
		const IConfigNode* const Item = Levels->GetItem(Index);
		const std::optional<std::string> Id = Item ? Item->ReadString() : std::nullopt;
		if ( Id && !Id->empty() && std::find(Result.LevelIds.begin(), Result.LevelIds.end(), *Id) == Result.LevelIds.end() )
		{
			Result.LevelIds.push_back(*Id);
		}
	}
	return Result;
}

} // namespace tanks::model
