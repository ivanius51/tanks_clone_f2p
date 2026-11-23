#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "model/ConfigNode.h"

namespace tanks::model
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
	// Reads a list of { "id", "name", "counter", "target" }; items without an id or a counter are skipped.
	void Load(const IConfigNode& InList);
	void SetAchievements(const std::vector<AchievementDef>& InAchievements);

	const std::vector<AchievementDef>& GetAchievements() const;
	const AchievementDef* Find(const std::string& InId) const;
	// Ids reached by the counters that are not in InUnlocked yet, in the order of the list.
	std::vector<std::string> FindReached(const std::map<std::string, int>& InCounters, const std::set<std::string>& InUnlocked) const;

private:
	std::vector<AchievementDef> Achievements;
};

} // namespace tanks::model
