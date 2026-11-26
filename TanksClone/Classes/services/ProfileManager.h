#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>

#include "Structs/PixelPoint.h"
#include "services/StringsManager.h"
#include "templates/TContainers.h"
#include "templates/TTemplateHelper.h"

namespace game
{
// Forward declarations
	enum class EWindowMode;	
	namespace model
	{
		enum class ETextSize;
	}
/** Everything that is remembered between runs. */
struct PlayerProfile
{
	int32 Coins;
	int32 BestScore;
	TMap<FPersistentString, int32> BestScores;
	int32 LevelsCompleted;
	TMap<FPersistentString, int32> Upgrades;
	TMap<FPersistentString, int32> Counters;
	std::set<std::string> Achievements;
	std::string Difficulty;
	TMap<int32, int32> LevelDeaths;
	std::string CurrentLanguage;
	bool bSoundEnabled;
	int32 FpsLimit;
	bool bVerticalSync;
	EWindowMode CurrentWindowMode;
	int32 MapScale;
	TOptional<PixelPoint> WindowPosition;
	bool bHdTextures;
	bool bSmoothPixels;
	int32 EffectsVolume;
	int32 MusicVolume;
	std::string AudioDevice;
	model::ETextSize CurrentTextSize;

	PlayerProfile();

	void ResetToDefaults();
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
} // namespace game
