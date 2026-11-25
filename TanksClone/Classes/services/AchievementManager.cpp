#include "services/AchievementManager.h"

#include "core/ConfigReader.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "services/ConfigRefs.h"
#include "services/ProfileManager.h"
#include "services/ResourceManager.h"

namespace game
{

void AchievementManager::Init()
{
	if ( const model::IConfigNode* const List = config_refs::FindChild(ids::configs::Achievements, "achievements") )
	{
		Book.Load(*List);
	}
}

std::vector<std::string> AchievementManager::AddToCounter(const std::string_view InCounter, const int InAmount)
{
	PlayerProfile& Profile = GameContext::Get<ProfileManager>().Get();
	Profile.Counters[std::string(InCounter)] += InAmount;
	const std::vector<std::string> Reached = Book.FindReached(Profile.Counters, Profile.Achievements);
	Profile.Achievements.insert(Reached.begin(), Reached.end());
	return Reached;
}

bool AchievementManager::IsUnlocked(const std::string& InId) const
{
	return GameContext::Get<ProfileManager>().Get().Achievements.count(InId) > 0;
}

const model::AchievementBook& AchievementManager::GetBook() const
{
	return Book;
}

} // namespace game
