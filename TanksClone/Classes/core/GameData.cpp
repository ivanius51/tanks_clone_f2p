#include "core/GameData.h"

#include "cocos2d.h"

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "game/components/Buff.h"
#include "game/EnemyCatalog.h"
#include "game/ResourceManifest.h"
#include "services/AtlasScope.h"
#include "services/AudioManager.h"
#include "services/ProfileManager.h"
#include "services/StringsManager.h"
#include "services/UpgradeManager.h"

namespace tanks
{

void LoadGameData(const bool bInTestMode)
{
	ConfigReader::GetInstance().ReadObjectConfigs(ConfigsFolder);
	StringsManager::GetInstance().Init();
	BuffCatalog::GetInstance().Init();
	EnemyCatalog::GetInstance().Init();
	UpgradeManager::GetInstance().Init();

	if ( bInTestMode )
	{
		// A fresh profile that is never saved.
		ProfileManager::GetInstance().Get() = PlayerProfile();
		ProfileManager::GetInstance().Get().bSoundEnabled = false;
		ProfileManager::GetInstance().Get().FpsLimit = 0; // as fast as possible: the level tests run in fast forward
		ProfileManager::GetInstance().Get().bVerticalSync = false;
		ProfileManager::GetInstance().ApplySettings();
		AtlasScope::KeepLoaded(ResourceManifest().GetGlobalAtlases());
		return;
	}

	ProfileManager::GetInstance().Load();
	// The texture set (HD or SD) is a setting, so the atlases load after the profile.
	ProfileManager::GetInstance().ApplySettings();
	AtlasScope::KeepLoaded(ResourceManifest().GetGlobalAtlases());
	ProfileManager::GetInstance().ApplyWindowSettings();
	ProfileManager::GetInstance().RememberWindowMoves();
	AudioManager::GetInstance().Preload();
}

} // namespace tanks
