#include "core/GameData.h"

#include "cocos2d.h"

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/components/Buff.h"
#include "game/EnemyCatalog.h"
#include "game/ResourceManifest.h"
#include "model/GraphicsQuality.h"
#include "platform/cocos/LegacyProfile.h"
#include "services/MusicManager.h"
#include "services/AudioManager.h"
#include "services/InputManager.h"
#include "services/ProfileManager.h"
#include "services/ResourceManager.h"
#include "services/GlyphCoverage.h"
#include "services/StringsManager.h"
#include "services/AchievementManager.h"
#include "services/DifficultyManager.h"
#include "services/LevelLegendManager.h"
#include "services/UpgradeManager.h"

namespace game
{

void LoadGameData(const bool bInTestMode)
{
	GameContext::Get<ConfigReader>().ReadObjectConfigs(ConfigsFolder);
	GameContext::Get<StringsManager>().Init();
	GlyphCoverage::ReportAll();
	GameContext::Get<BuffCatalog>().Init();
	GameContext::Get<EnemyCatalog>().Init();
	GameContext::Get<AchievementManager>().Init();
	GameContext::Get<DifficultyManager>().Init();
	GameContext::Get<LevelLegendManager>().Init();
	GameContext::Get<UpgradeManager>().Init();
	GameContext::Get<InputManager>().Init();
	GameContext::Get<MusicManager>().Init();

	if ( bInTestMode )
	{
		// A fresh profile that is never saved.
		GameContext::Get<ProfileManager>().Get() = PlayerProfile();
		GameContext::Get<ProfileManager>().Get().bSoundEnabled = false;
		GameContext::Get<ProfileManager>().Get().FpsLimit = 0; // as fast as possible: the level tests run in fast forward
		GameContext::Get<ProfileManager>().Get().bVerticalSync = false;
		GameContext::Get<ProfileManager>().Get().GraphicsQualityLevel = static_cast<int>(model::GraphicsQuality::Middle);
		GameContext::Get<ProfileManager>().ApplySettings();
		GameContext::Get<ResourceManager>().KeepAtlasesLoaded(ResourceManifest().GetGlobalAtlases());
		return;
	}

	view::cocos::MigrateLegacyProfile(GameContext::Get().GetStorage());
	GameContext::Get<ProfileManager>().Load();
	// The texture set (HD or SD) is a setting, so the atlases load after the profile.
	GameContext::Get<ProfileManager>().ApplySettings();
	GameContext::Get<ResourceManager>().KeepAtlasesLoaded(ResourceManifest().GetGlobalAtlases());
	GameContext::Get<ProfileManager>().ApplyWindowSettings();
	GameContext::Get<ProfileManager>().RememberWindowMoves();
	GameContext::Get<AudioManager>().Preload();
}

} // namespace game
