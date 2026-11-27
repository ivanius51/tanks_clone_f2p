#pragma once

#include "templates/TManagerSet.h"
#include "templates/TTypeList.h"
#include "templates/TTemplateHelper.h"

namespace game
{
namespace ports
{
class IAudioOutput;
class IInputSource;
class IStorage;
}

namespace view::cocos
{
class CocosAudioOutput;
class CocosClockDriver;
class CocosKeyboardInput;
class CocosStorage;
}

namespace model
{
class GameEvents;
class Scheduler;
class TimerManager;
class SimulationClock;
}

class AudioManager;
class MusicManager;
class BuffCatalog;
class CommandsManager;
class ConfigReader;
class EnemyCatalog;
class HoverManager;
class InputManager;
class PlayerInputController;
class ObjectsFactory;
class PhysicsManager;
class ProfileManager;
class ResourceManager;
class StringsManager;
class UiStyle;
class UpgradeManager;
class AchievementManager;
class DifficultyManager;
class LevelLegendManager;
class GroundManager;

class Tank;

using PortAdapters = TTypeList<view::cocos::CocosStorage, view::cocos::CocosAudioOutput, view::cocos::CocosKeyboardInput>;

using GameManagers = TTypeList<model::SimulationClock, model::Scheduler, model::TimerManager, model::GameEvents, view::cocos::CocosClockDriver, ConfigReader, StringsManager, ProfileManager, ResourceManager, UiStyle, AudioManager, MusicManager, BuffCatalog, EnemyCatalog,
	AchievementManager, DifficultyManager, LevelLegendManager, GroundManager, UpgradeManager, PhysicsManager, ObjectsFactory, CommandsManager, InputManager, HoverManager, PlayerInputController>;

/**
 * Owns every game manager and their lifetime. The entry point owns the context; the platform layer calls
 * Initialize() once the engine runs and Shutdown() before the engine is torn down.
 * The ports (storage, audio output, input) are reached through the getters below.
 * Get() is the single global access point, for the View and code not yet migrated; the Model takes its
 * dependencies explicitly.
 */
class GameContext : public FNonCopyableNonMovable
{
public:
	GameContext();
	~GameContext();

	static GameContext& Get();
	static Tank* GetPlayer();

	void Initialize();
	void Shutdown();
	bool IsInitialized() const;

	ports::IStorage& GetStorage() const;
	ports::IAudioOutput& GetAudioOutput() const;
	ports::IInputSource& GetInputSource() const;

	template <typename TManager>
	static TManager& Get()
	{
		return GameContext::Get().GetManager<TManager>();
	}

	template <typename TManager>
	TManager& GetManager() const
	{
		if constexpr (GameManagers::Contains<TManager>)
		{
			return Managers.Get<TManager>();
		}
		else if constexpr (PortAdapters::Contains<TManager>)
		{
			return Adapters.Get<TManager>();
		}
		else
		{
			static_assert(TAlwaysFalse<TManager>, "TManager is not registered in GameManagers or PortAdapters");
		}
	}

private:
	TManagerSet<PortAdapters> Adapters;
	TManagerSet<GameManagers> Managers;
};
} // namespace game
