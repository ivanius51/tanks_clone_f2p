#include "services/ProfileManager.h"

#include "core/DataConverter.h"
#include "core/GameContext.h"
#include "services/ResourceManager.h"
#include "services/AudioManager.h"

namespace tanks
{

namespace
{
	constexpr const char* ProfileFile = "profile.plist";

	std::string ProfilePath()
	{
		return cocos2d::FileUtils::getInstance()->getWritablePath() + ProfileFile;
	}
}

PlayerProfile& ProfileManager::Get()
{
	return Profile;
}

const PlayerProfile& ProfileManager::Get() const
{
	return Profile;
}

void ProfileManager::Load()
{
	Profile = PlayerProfile();

	auto FileUtilsInstance = cocos2d::FileUtils::getInstance();
	if ( !FileUtilsInstance->isFileExist(ProfilePath()) )
	{
		return;
	}

	const cocos2d::ValueMap ConfigData = FileUtilsInstance->getValueMapFromFile(ProfilePath());
	Profile.Coins = DataConverter::GetInt(ConfigData, "coins", 0);
	Profile.BestScore = DataConverter::GetInt(ConfigData, "best_score", 0);
	Profile.LevelsCompleted = DataConverter::GetInt(ConfigData, "levels_completed", 0);
	Profile.bSoundEnabled = DataConverter::GetBool(ConfigData, "sound", true);
	Profile.FpsLimit = DataConverter::GetInt(ConfigData, "fps", 60);
	Profile.bVerticalSync = DataConverter::GetBool(ConfigData, "vsync", true);
	Profile.CurrentWindowMode = DataConverter::GetInt(ConfigData, "window_mode", 0) == static_cast<int>(WindowMode::Borderless)
		? WindowMode::Borderless : WindowMode::Windowed;
	Profile.WindowScale = DataConverter::GetInt(ConfigData, "window_scale", 100);
	Profile.bSmoothPixels = DataConverter::GetBool(ConfigData, "smooth_pixels", false);
	Profile.bHdTextures = DataConverter::GetBool(ConfigData, "hd_textures", true);
	if ( ConfigData.count("window_x") > 0 && ConfigData.count("window_y") > 0 )
	{
		Profile.WindowPosition = PixelPoint{ DataConverter::GetInt(ConfigData, "window_x", 0), DataConverter::GetInt(ConfigData, "window_y", 0) };
	}
	Profile.EffectsVolume = DataConverter::GetInt(ConfigData, "effects_volume", 100);

	const int StoredLanguage = DataConverter::GetInt(ConfigData, "language", static_cast<int>(Language::English));
	const bool bKnown = StoredLanguage == static_cast<int>(Language::Russian) || StoredLanguage == static_cast<int>(Language::Ukrainian);
	Profile.CurrentLanguage = bKnown ? static_cast<Language>(StoredLanguage) : Language::English;

	auto UpgradesIt = ConfigData.find("upgrades");
	if ( UpgradesIt != ConfigData.end() && UpgradesIt->second.getType() == cocos2d::Value::Type::MAP )
	{
		for ( const auto& Upgrade : UpgradesIt->second.asValueMap() )
		{
			Profile.Upgrades[Upgrade.first] = Upgrade.second.asInt();
		}
	}
}

void ProfileManager::Save() const
{
	cocos2d::ValueMap ConfigData;
	ConfigData["coins"] = Profile.Coins;
	ConfigData["best_score"] = Profile.BestScore;
	ConfigData["levels_completed"] = Profile.LevelsCompleted;
	ConfigData["sound"] = Profile.bSoundEnabled;
	ConfigData["fps"] = Profile.FpsLimit;
	ConfigData["language"] = static_cast<int>(Profile.CurrentLanguage);
	ConfigData["vsync"] = Profile.bVerticalSync;
	ConfigData["window_mode"] = static_cast<int>(Profile.CurrentWindowMode);
	ConfigData["window_scale"] = Profile.WindowScale;
	ConfigData["smooth_pixels"] = Profile.bSmoothPixels;
	ConfigData["hd_textures"] = Profile.bHdTextures;
	if ( Profile.WindowPosition )
	{
		ConfigData["window_x"] = Profile.WindowPosition->X;
		ConfigData["window_y"] = Profile.WindowPosition->Y;
	}
	ConfigData["effects_volume"] = Profile.EffectsVolume;

	cocos2d::ValueMap CurrentUpgrades;
	for ( const auto& Upgrade : Profile.Upgrades )
	{
		CurrentUpgrades[Upgrade.first] = Upgrade.second;
	}
	ConfigData["upgrades"] = CurrentUpgrades;

	if ( !cocos2d::FileUtils::getInstance()->writeValueMapToFile(ConfigData, ProfilePath()) )
	{
		CCLOGERROR("ProfileManager: could not save %s", ProfilePath().c_str());
	}
}

void ProfileManager::ResetProgress()
{
	PlayerProfile Fresh;
	Fresh.CurrentLanguage = Profile.CurrentLanguage;
	Fresh.bSoundEnabled = Profile.bSoundEnabled;
	Fresh.FpsLimit = Profile.FpsLimit;
	Fresh.bVerticalSync = Profile.bVerticalSync;
	Fresh.CurrentWindowMode = Profile.CurrentWindowMode;
	Fresh.WindowScale = Profile.WindowScale;
	Fresh.bSmoothPixels = Profile.bSmoothPixels;
	Fresh.bHdTextures = Profile.bHdTextures;
	Fresh.WindowPosition = Profile.WindowPosition;
	Fresh.EffectsVolume = Profile.EffectsVolume;
	Profile = Fresh;
}

bool ProfileManager::HasProgress() const
{
	return Profile.LevelsCompleted > 0 || Profile.Coins > 0 || !Profile.Upgrades.empty();
}

void ProfileManager::ApplySettings() const
{
	GameContext::Get().GetManager<StringsManager>().SetLanguage(Profile.CurrentLanguage);
	GameContext::Get().GetManager<AudioManager>().SetEnabled(Profile.bSoundEnabled);
	GameContext::Get().GetManager<AudioManager>().SetVolume(Profile.EffectsVolume / 100.0f);
	SetVerticalSyncEnabled(Profile.bVerticalSync);
	GameContext::Get().GetManager<ResourceManager>().SetSmoothPixels(Profile.bSmoothPixels);
	ApplyHdTextures(Profile.bHdTextures);
}

void ProfileManager::ApplyWindowSettings() const
{
	ApplyWindowMode(Profile.CurrentWindowMode, Profile.WindowScale, Profile.WindowPosition);
}

void ProfileManager::RememberWindowMoves()
{
	WatchWindowMoves([this](const PixelPoint InPosition)
	{
		if ( Profile.CurrentWindowMode != WindowMode::Windowed )
		{
			return;
		}
		Profile.WindowPosition = InPosition;
		Save();
	});
}

} // namespace tanks
