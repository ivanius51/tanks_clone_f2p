#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>

#include "model/TextSize.h"
#include "services/DisplaySettings.h"
#include "services/StringsManager.h"

namespace tanks
{
/** Everything that is remembered between runs. */
struct PlayerProfile
{
	int Coins = 0;
	int BestScore = 0;
	std::map<std::string, int> BestScores;
	int LevelsCompleted = 0;
	std::map<std::string, int> Upgrades;
	std::map<std::string, int> Counters;
	std::set<std::string> Achievements;
	std::string Difficulty = "normal";
	std::map<int, int> LevelDeaths;
	std::string CurrentLanguage;
	bool bSoundEnabled = true;
	int FpsLimit = 60;
	bool bVerticalSync = true;
	WindowMode CurrentWindowMode = WindowMode::Windowed;
	int MapScale = 0;
	std::optional<PixelPoint> WindowPosition;
	bool bHdTextures = true;
	bool bSmoothPixels = false;
	int EffectsVolume = 100;
	int MusicVolume = 60;
	std::string AudioDevice;
	model::TextSize CurrentTextSize = model::TextSize::Small;
};

/** Loads and saves the player profile (the file "profile.json" of the storage port). */
class ProfileManager
{
public:
	ProfileManager() = default;
	~ProfileManager() = default;
	ProfileManager(const ProfileManager&) = delete;
	ProfileManager& operator=(const ProfileManager&) = delete;

	PlayerProfile& Get();
	const PlayerProfile& Get() const;

	void Load();
	void Save() const;

	void ResetProgress();
	bool HasProgress() const;

	void ApplySettings() const;
	void ApplyWindowSettings() const;
	void RememberWindowMoves();

private:
	PlayerProfile Profile;
};
} // namespace tanks
