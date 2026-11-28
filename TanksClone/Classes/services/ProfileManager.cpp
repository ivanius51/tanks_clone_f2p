#include "services/ProfileManager.h"

#include "scenes/TypedScene.h"

#include "core/GameContext.h"
#include "core/JsonConfigNode.h"
#include "model/Log.h"
#include "ports/IStorage.h"
#include "services/AudioManager.h"
#include "services/MusicManager.h"
#include "services/ProfileSerializer.h"
#include "services/ResourceManager.h"
#include "model/TextSize.h"
#include "services/DisplaySettings.h"

namespace game
{

namespace
{
	constexpr const char* ProfileFile = "profile.json";
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

	const std::optional<std::string> Text = GameContext::Get().GetStorage().ReadText(ProfileFile);
	if ( !Text )
	{
		return;
	}
	bool bOk = false;
	const auto Root{ JsonConfigNode::Parse(*Text, ProfileFile, &bOk) };
	if ( bOk )
	{
		Profile = ProfileSerializer::FromJson(*Root);
	}
}

void ProfileManager::Save() const
{
	if ( !GameContext::Get().GetStorage().WriteText(ProfileFile, ProfileSerializer::ToJson(Profile)) )
	{
		model::Log::Error("ProfileManager: could not save %s", ProfileFile);
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
	Fresh.MapScale = Profile.MapScale;
	Fresh.bSmoothPixels = Profile.bSmoothPixels;
	Fresh.bSmoothWalls = Profile.bSmoothWalls;
	Fresh.bTiltedTanks = Profile.bTiltedTanks;
	Fresh.GraphicsQualityLevel = Profile.GraphicsQualityLevel;
	Fresh.TextureQuality = Profile.TextureQuality;
	Fresh.WindowPosition = Profile.WindowPosition;
	Fresh.EffectsVolume = Profile.EffectsVolume;
	Fresh.MusicVolume = Profile.MusicVolume;
	Fresh.AudioDevice = Profile.AudioDevice;
	Profile = Fresh;
}

bool ProfileManager::HasProgress() const
{
	return Profile.LevelsCompleted > 0 || Profile.Coins > 0 || !Profile.Upgrades.IsEmpty() || !Profile.Counters.IsEmpty();
}

void ProfileManager::ApplySettings() const
{
	GameContext::Get<StringsManager>().SetLanguage(Profile.CurrentLanguage);
	GameContext::Get<AudioManager>().SetEnabled(Profile.bSoundEnabled);
	GameContext::Get<AudioManager>().SetVolume(Profile.EffectsVolume / 100.0f);
	GameContext::Get<AudioManager>().SetDevice(Profile.AudioDevice);
	GameContext::Get<MusicManager>().SetVolume(Profile.MusicVolume / 100.0f);
	SetVerticalSyncEnabled(Profile.bVerticalSync);
	GameContext::Get<ResourceManager>().SetSmoothPixels(Profile.bSmoothPixels);
	ApplyTextureSettings();
}

void ProfileManager::ApplyTextureSettings() const
{
	// Sprites made before a swap keep the old frame offsets, so the set never changes while a level or the editor is open.
	const cocos2d::Scene* const Running = cocos2d::Director::getInstance()->getRunningScene();
	using view::cocos::SceneKind;
	if ( Running && (Running->getTag() == view::cocos::TypedScene<SceneKind::Game>::SceneTag(SceneKind::Game)
		|| Running->getTag() == view::cocos::TypedScene<SceneKind::Editor>::SceneTag(SceneKind::Editor)) )
	{
		return;
	}
	const float Zoom = Profile.MapScale > 0 ? static_cast<float>(Profile.MapScale) / 100.0f : 1.0f;
	ApplyTextureQualitySetting(Profile.TextureQuality, Zoom);
}

void ProfileManager::ApplyWindowSettings() const
{
	ApplyWindowMode(Profile.CurrentWindowMode, Profile.WindowPosition);
}

void ProfileManager::RememberWindowMoves()
{
	WatchWindowMoves([this](const PixelPoint InPosition)
	{
		if ( Profile.CurrentWindowMode != EWindowMode::Windowed )
		{
			return;
		}
		Profile.WindowPosition = InPosition;
		Save();
	});
}

PlayerProfile::PlayerProfile()
{
	ResetToDefaults();
}

void PlayerProfile::ResetToDefaults()
{
	Coins = 0;
	BestScore = 0;
	BestScores.Clear();
	LevelsCompleted = 0;
	Upgrades.Clear();
	Counters.Clear();
	Achievements.Clear();
	Difficulty = "normal";
	LevelDeaths.Clear();
	CurrentLanguage = std::string(GameContext::Get<StringsManager>().GetDefaultLanguageId().View());
	bSoundEnabled = true;
	FpsLimit = 60;
	bVerticalSync = true;
	CurrentWindowMode = EWindowMode::Windowed;
	MapScale = 140;
	WindowPosition.Reset();
	TextureQuality = AutoTextureQuality;
	bSmoothPixels = false;
	bSmoothWalls = true;
	bTiltedTanks = true;
	GraphicsQualityLevel = -1;
	EffectsVolume = 100;
	MusicVolume = 60;
	AudioDevice.clear();
	CurrentTextSize = model::ETextSize::Small;
}

} // namespace game
