#include "services/ProfileManager.h"

#include "core/GameContext.h"
#include "core/JsonConfigNode.h"
#include "ports/IStorage.h"
#include "services/AudioManager.h"
#include "services/ProfileSerializer.h"
#include "services/ResourceManager.h"

namespace tanks
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
	const std::unique_ptr<JsonConfigNode> Root = JsonConfigNode::Parse(*Text, ProfileFile, &bOk);
	if ( bOk )
	{
		Profile = ProfileSerializer::FromJson(*Root);
	}
}

void ProfileManager::Save() const
{
	if ( !GameContext::Get().GetStorage().WriteText(ProfileFile, ProfileSerializer::ToJson(Profile)) )
	{
		CCLOGERROR("ProfileManager: could not save %s", ProfileFile);
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
