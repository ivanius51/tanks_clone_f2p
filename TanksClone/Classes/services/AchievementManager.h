#pragma once

#include <string>
#include <string_view>

#include "templates/TTemplateHelper.h"
#include "model/AchievementBook.h"
#include "templates/TContainers.h"

namespace game
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
class AchievementManager : public FNonCopyableNonMovable
{
public:
	AchievementManager() = default;
	~AchievementManager() = default;

	void Init();

	TArray<std::string> AddToCounter(std::string_view InCounter, int InAmount);
	bool IsUnlocked(const std::string& InId) const;
	const model::AchievementBook& GetBook() const;

private:
	model::AchievementBook Book;
};
} // namespace game
