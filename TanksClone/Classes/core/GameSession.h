#pragma once

#include <string>

namespace tanks::model
{

// What has happened during the current run of a level.
struct GameSession
{
	int Score = 0;
	int Kills = 0;
	int Coins = 0;
	int Drops = 0;           // pickups enemies dropped
	int DropsPicked = 0;
	int TotalEnemies = 0;
	int WaveNumber = 0;      // current wave (from 1), 0 before the first one
	int WaveCount = 0;
	std::string LevelName; // already translated
};

} // namespace tanks::model