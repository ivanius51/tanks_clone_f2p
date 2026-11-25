#pragma once

#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "templates/TStoredString.h"

namespace game::model
{
/** One thing that happened: its name ("pickup.taken", "enemy.spawned") and the game time it was reported. */
struct GameEvent
{
	FTransientString Name;
	double Time = 0.0;
};

/**
 * The log of what happened in the current level, reported by the game code and read by whoever reacts: level scripts
 * ("until": "pickup.taken"), achievements, the bot, tests. Engine free; Clear starts a new level.
 */
class GameEvents
{
public:
	void Emit(std::string_view InName, double InTime);
	void Clear();

	bool Has(std::string_view InName) const;
	int GetCount(std::string_view InName) const;
	const std::vector<GameEvent>& GetLog() const;

private:
	std::vector<GameEvent> Log;
	std::map<FTransientString, int> Counts;
};
} // namespace game::model
