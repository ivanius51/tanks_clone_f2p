#include "model/AchievementBook.h"
#include "templates/TContainers.h"

namespace game::model
{

void AchievementBook::Load(const IConfigNode& InList)
{
	Achievements.ReadList(InList, [](const IConfigNode& InItem, AchievementDef& OutDef)
	{
		if ( !InItem.TryRead("id", OutDef.Id) || !InItem.TryRead("counter", OutDef.Counter) )
		{
			return false;
		}
		OutDef.NameKey = InItem.GetValueOr("name", OutDef.Id.c_str());
		OutDef.Target = InItem.GetValueOr("target", OutDef.Target);
		return true;
	});
}

void AchievementBook::SetAchievements(const TArray<AchievementDef>& InAchievements)
{
	Achievements.SetAll(InAchievements);
}

const TArray<AchievementDef>& AchievementBook::GetAchievements() const
{
	return Achievements.GetAll();
}

const AchievementDef* AchievementBook::Find(const std::string& InId) const
{
	return Achievements.Find(InId);
}

TArray<std::string> AchievementBook::FindReached(const TMap<FPersistentString, int>& InCounters, const TSet<FPersistentString>& InUnlocked) const
{
	TArray<std::string> Reached;
	for ( const AchievementDef& Achievement : Achievements.GetAll() )
	{
		auto* const Counter = InCounters.Find(Achievement.Counter);
		const int Value = Counter ? *Counter : 0;
		if ( Value >= Achievement.Target && !InUnlocked.Contains(Achievement.Id) )
		{
			Reached.PushBack(Achievement.Id);
		}
	}
	return Reached;
}

} // namespace game::model
