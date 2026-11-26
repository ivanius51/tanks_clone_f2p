#pragma once

#include <map>
#include <string>
#include <vector>

#include "model/ConfigCatalog.h"
#include "templates/TContainers.h"

namespace game::model
{
/** One achievement: reached once the named counter of the profile gets to Target. */
struct AchievementDef
{
	std::string Id;
	std::string NameKey;
	std::string Counter;
	int Target = 1;
};

/**
 * The list of achievements (configs/balance/achievements.json) and the rule that reaches them: a counter of the player
 * (kills, levels won, perfect levels, coins earned...) at or above the target. Engine free, so it is unit-tested.
 */
class AchievementBook
{
public:
	void Load(const IConfigNode& InList);
	void SetAchievements(const std::vector<AchievementDef>& InAchievements);

	const std::vector<AchievementDef>& GetAchievements() const;
	const AchievementDef* Find(const std::string& InId) const;
	std::vector<std::string> FindReached(const TMap<FPersistentString, int>& InCounters, const TSet<FPersistentString>& InUnlocked) const;

private:
	ConfigCatalog<AchievementDef> Achievements;
};
} // namespace game::model
