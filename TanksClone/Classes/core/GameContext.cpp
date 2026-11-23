#include "core/GameContext.h"

#include <cassert>

#include "core/ConfigReader.h"
#include "game/CommandsManager.h"
#include "game/components/Buff.h"
#include "game/EnemyCatalog.h"
#include "game/PlayerInputController.h"
#include "platform/cocos/CocosAudioOutput.h"
#include "platform/cocos/CocosKeyboardInput.h"
#include "platform/cocos/CocosStorage.h"
#include "game/ObjectsFactory.h"
#include "services/AudioManager.h"
#include "services/MusicManager.h"
#include "services/InputManager.h"
#include "services/ProfileManager.h"
#include "services/ResourceManager.h"
#include "services/StringsManager.h"
#include "services/UiStyle.h"
#include "services/AchievementManager.h"
#include "services/DifficultyManager.h"
#include "services/LevelLegendManager.h"
#include "services/UpgradeManager.h"

namespace tanks
{

namespace
{
	GameContext* GCurrent = nullptr;
}

GameContext::GameContext()
{
	assert(GCurrent == nullptr && "only one game context exists at a time");
	GCurrent = this;
}

GameContext::~GameContext()
{
	Shutdown();
	GCurrent = nullptr;
}

GameContext& GameContext::Get()
{
	assert(GCurrent != nullptr && "the entry point owns the game context");
	return *GCurrent;
}

void GameContext::Initialize()
{
	Adapters.Create();
	Managers.Create();
}

void GameContext::Shutdown()
{
	Managers.Destroy();
	Adapters.Destroy();
}

bool GameContext::IsInitialized() const
{
	return Managers.IsCreated();
}

ports::IStorage& GameContext::GetStorage() const
{
	return Adapters.Get<view::cocos::CocosStorage>();
}

ports::IAudioOutput& GameContext::GetAudioOutput() const
{
	return Adapters.Get<view::cocos::CocosAudioOutput>();
}

ports::IInputSource& GameContext::GetInputSource() const
{
	return Adapters.Get<view::cocos::CocosKeyboardInput>();
}

} // namespace tanks
