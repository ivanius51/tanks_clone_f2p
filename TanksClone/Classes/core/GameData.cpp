#include "core/GameData.h"

#include "cocos2d.h"

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/components/Buff.h"
#include "game/EnemyCatalog.h"
#include "game/ResourceManifest.h"
#include "services/AudioManager.h"
#include "services/ProfileManager.h"
#include "services/ResourceManager.h"
#include "services/StringsManager.h"
#include "services/UpgradeManager.h"

namespace tanks
{

void LoadGameData(const bool bInTestMode)
{
	GameContext::Get().GetManager<ConfigReader>().ReadObjectConfigs(ConfigsFolder);
	GameContext::Get().GetManager<StringsManager>().Init();
	GameContext::Get().GetManager<BuffCatalog>().Init();
	GameContext::Get().GetManager<EnemyCatalog>().Init();
	GameContext::Get().GetManager<UpgradeManager>().Init();

	if ( bInTestMode )
	{
		// A fresh profile that is never saved.
		GameContext::Get().GetManager<ProfileManager>().Get() = PlayerProfile();
		GameContext::Get().GetManager<ProfileManager>().Get().bSoundEnabled = false;
		GameContext::Get().GetManager<ProfileManager>().Get().FpsLimit = 0; // as fast as possible: the level tests run in fast forward
		GameContext::Get().GetManager<ProfileManager>().Get().bVerticalSync = false;
		GameContext::Get().GetManager<ProfileManager>().ApplySettings();
		GameContext::Get().GetManager<ResourceManager>().KeepAtlasesLoaded(ResourceManifest().GetGlobalAtlases());
		return;
	}

	GameContext::Get().GetManager<ProfileManager>().Load();
	// The texture set (HD or SD) is a setting, so the atlases load after the profile.
	GameContext::Get().GetManager<ProfileManager>().ApplySettings();
	GameContext::Get().GetManager<ResourceManager>().KeepAtlasesLoaded(ResourceManifest().GetGlobalAtlases());
	GameContext::Get().GetManager<ProfileManager>().ApplyWindowSettings();
	GameContext::Get().GetManager<ProfileManager>().RememberWindowMoves();
	GameContext::Get().GetManager<AudioManager>().Preload();
}

} // namespace tanks
