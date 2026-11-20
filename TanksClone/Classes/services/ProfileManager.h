#pragma once

#include <map>
#include <optional>
#include <string>

#include "services/DisplaySettings.h"
#include "services/StringsManager.h"

namespace tanks
{

// Everything that is remembered between runs.
struct PlayerProfile
{
	int Coins = 0;
	int BestScore = 0;
	int LevelsCompleted = 0;
	std::map<std::string, int> Upgrades; // upgrade id -> level
	Language CurrentLanguage = Language::English;
	bool bSoundEnabled = true;
	int FpsLimit = 60;      // 0 = unlimited
	bool bVerticalSync = true;
	WindowMode CurrentWindowMode = WindowMode::Windowed;
	int WindowScale = 100;  // percent of the design size; 0 = auto
	std::optional<PixelPoint> WindowPosition; // the windowed window's outer top-left corner; none = centred
	bool bHdTextures = true;
	bool bSmoothPixels = false;
	int EffectsVolume = 100; // percent
};

// Loads and saves the player profile (a plist file in the writable folder).
class ProfileManager
{
public:
	static ProfileManager& GetInstance();

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
	ProfileManager() = default;
	~ProfileManager() = default;
	ProfileManager(const ProfileManager&) = delete;
	ProfileManager& operator=(const ProfileManager&) = delete;

	PlayerProfile Profile;
};

} // namespace tanks
