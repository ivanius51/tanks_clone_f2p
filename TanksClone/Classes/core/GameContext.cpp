#include "core/GameContext.h"

#include <algorithm>
#include <cassert>

#include "core/ConfigReader.h"
#include "game/CommandsManager.h"
#include "game/components/Buff.h"
#include "game/EnemyCatalog.h"
#include "game/PlayerInputController.h"
#include "platform/cocos/CocosAudioOutput.h"
#include "platform/cocos/CocosClockDriver.h"
#include "platform/cocos/CocosKeyboardInput.h"
#include "platform/cocos/CocosStorage.h"
#include "game/ObjectsFactory.h"
#include "game/PhysicsManager.h"
#include "model/GameEvents.h"
#include "model/Scheduler.h"
#include "model/SimulationClock.h"
#include "model/TimerManager.h"
#include "services/AudioManager.h"
#include "services/MusicManager.h"
#include "services/HoverManager.h"
#include "services/InputManager.h"
#include "services/ProfileManager.h"
#include "services/ResourceManager.h"
#include "services/StringsManager.h"
#include "services/UiStyle.h"
#include "services/AchievementManager.h"
#include "services/DifficultyManager.h"
#include "services/GroundManager.h"
#include "services/LevelLegendManager.h"
#include "services/UpgradeManager.h"
#include "game/objects/Tank.h"
#include "core/GameConstants.h"

namespace game
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

Tank* GameContext::GetPlayer()
{
    return GameContext::Get<ObjectsFactory>().FindObject<Tank>(PlayerName).Get();
}

void GameContext::Tick(const float InDeltaMs, const std::chrono::microseconds InBudget)
{
	const auto Start = std::chrono::steady_clock::now();
	TickableManagers::ForEach([this, InDeltaMs, InBudget, Start](auto InTag)
	{
		std::chrono::microseconds Left = InBudget;
		if ( InBudget != std::chrono::microseconds::max() )
		{
			Left = std::max(std::chrono::microseconds(0), InBudget - std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - Start));
		}
		GetManager<typename decltype(InTag)::Type>().Tick(InDeltaMs, Left);
	});
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

} // namespace game
