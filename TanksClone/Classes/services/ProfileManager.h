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

// Everything that is remembered between runs.
struct PlayerProfile
{
	int Coins = 0;
	int BestScore = 0;
	std::map<std::string, int> BestScores; // difficulty id -> best score in that mode
	int LevelsCompleted = 0;
	std::map<std::string, int> Upgrades; // upgrade id -> level
	std::map<std::string, int> Counters; // what the player has done: kills, levels won... (achievements check these)
	std::set<std::string> Achievements;
	std::string Difficulty = "normal";
	std::map<int, int> LevelDeaths; // level index -> deaths since it was last won or repaired
	std::string CurrentLanguage; // id from configs/strings/languages.json; empty = the default language
	bool bSoundEnabled = true;
	int FpsLimit = 60;      // 0 = unlimited
	bool bVerticalSync = true;
	WindowMode CurrentWindowMode = WindowMode::Windowed;
	int MapScale = 0;       // percent of the map's own size; 0 = auto (the whole map fits the window)
	std::optional<PixelPoint> WindowPosition; // the windowed window's outer top-left corner; none = centred
	bool bHdTextures = true;
	bool bSmoothPixels = false;
	int EffectsVolume = 100; // percent
	int MusicVolume = 60; // percent
	std::string AudioDevice;
	model::TextSize CurrentTextSize = model::TextSize::Small;
};

// Loads and saves the player profile (the file "profile.json" of the storage port).
class ProfileManager
{
public:
	ProfileManager() = default;
	~ProfileManager() = default;
	ProfileManager(const ProfileManager&) = delete;
	ProfileManager& operator=(const ProfileManager&) = delete;

	PlayerProfile& Get();
	const PlayerProfile& Get() const;

	// Missing or damaged files give a fresh profile.
	void Load();
	void Save() const;

	// Starts the game over: progress, coins and upgrades are lost, settings are kept.
	void ResetProgress();
	bool HasProgress() const;

	// Applies the settings of the profile to the game: language, sound and volume, vertical sync, pixel filtering,
	// HD or SD textures.
	// The frame rate limit is applied by the level itself.
	void ApplySettings() const;
	// Applies the window mode and size (separate: changing the window is visible, so only when asked).
	void ApplyWindowSettings() const;
	// Remembers where the player leaves the window (windowed mode only) and saves it.
	void RememberWindowMoves();

private:
	PlayerProfile Profile;
};

} // namespace tanks
