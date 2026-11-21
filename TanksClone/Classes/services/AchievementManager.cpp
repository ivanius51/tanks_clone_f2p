#include "services/AchievementManager.h"

#include "core/ConfigReader.h"
#include "core/GameContext.h"
#include "core/ResourceIds.h"
#include "services/ProfileManager.h"
#include "services/ResourceManager.h"

namespace tanks
{

void AchievementManager::Init()
{
	const std::string Path = GameContext::Get().GetManager<ResourceManager>().ResolvePath(ids::configs::Achievements, model::ResourceKind::Config);
	if ( const model::IConfigNode* const List = GameContext::Get().GetManager<ConfigReader>().GetConfig(Path).FindChild("achievements") )
	{
		Book.Load(*List);
	}
}

std::vector<std::string> AchievementManager::AddToCounter(const std::string_view InCounter, const int InAmount)
{
	PlayerProfile& Profile = GameContext::Get().GetManager<ProfileManager>().Get();
	Profile.Counters[std::string(InCounter)] += InAmount;
	const std::vector<std::string> Reached = Book.FindReached(Profile.Counters, Profile.Achievements);
	Profile.Achievements.insert(Reached.begin(), Reached.end());
	return Reached;
}

bool AchievementManager::IsUnlocked(const std::string& InId) const
{
	return GameContext::Get().GetManager<ProfileManager>().Get().Achievements.count(InId) > 0;
}

const model::AchievementBook& AchievementManager::GetBook() const
{
	return Book;
}

} // namespace tanks
