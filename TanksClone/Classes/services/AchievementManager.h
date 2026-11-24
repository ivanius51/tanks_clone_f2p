#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "model/AchievementBook.h"

namespace tanks
{
namespace counters
{
	inline constexpr std::string_view Kills = "kills";
	inline constexpr std::string_view LevelsWon = "levels_won";
	inline constexpr std::string_view PerfectLevels = "perfect_levels";
	inline constexpr std::string_view CoinsEarned = "coins_earned";
}

/**
 * Counts what the player has done (counters kept in the profile) and unlocks the achievements of
 * configs/balance/achievements.json when a counter reaches its target. Hidden upgrades wait for these.
 */
class AchievementManager
{
public:
	AchievementManager() = default;
	~AchievementManager() = default;
	AchievementManager(const AchievementManager&) = delete;
	AchievementManager& operator=(const AchievementManager&) = delete;

	void Init();

	std::vector<std::string> AddToCounter(std::string_view InCounter, int InAmount);
	bool IsUnlocked(const std::string& InId) const;
	const model::AchievementBook& GetBook() const;

private:
	model::AchievementBook Book;
};
} // namespace tanks
