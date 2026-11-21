#include "model/AchievementBook.h"

namespace tanks::model
{

void AchievementBook::Load(const IConfigNode& InList)
{
	Achievements.clear();
	for ( std::size_t Index = 0; Index < InList.GetItemCount(); ++Index )
	{
		const IConfigNode& Item = *InList.GetItem(Index);
		AchievementDef Achievement;
		if ( Item.TryRead("id", Achievement.Id) && Item.TryRead("counter", Achievement.Counter) )
		{
			Achievement.NameKey = Item.GetValueOr("name", Achievement.Id.c_str());
			Achievement.Target = Item.GetValueOr("target", Achievement.Target);
			Achievements.push_back(Achievement);
		}
	}
}

void AchievementBook::SetAchievements(const std::vector<AchievementDef>& InAchievements)
{
	Achievements = InAchievements;
}

const std::vector<AchievementDef>& AchievementBook::GetAchievements() const
{
	return Achievements;
}

const AchievementDef* AchievementBook::Find(const std::string& InId) const
{
	for ( const AchievementDef& Achievement : Achievements )
	{
		if ( Achievement.Id == InId )
		{
			return &Achievement;
		}
	}
	return nullptr;
}

std::vector<std::string> AchievementBook::FindReached(const std::map<std::string, int>& InCounters, const std::set<std::string>& InUnlocked) const
{
	std::vector<std::string> Reached;
	for ( const AchievementDef& Achievement : Achievements )
	{
		const auto Counter = InCounters.find(Achievement.Counter);
		const int Value = Counter != InCounters.end() ? Counter->second : 0;
		if ( Value >= Achievement.Target && InUnlocked.count(Achievement.Id) == 0 )
		{
			Reached.push_back(Achievement.Id);
		}
	}
	return Reached;
}

} // namespace tanks::model
