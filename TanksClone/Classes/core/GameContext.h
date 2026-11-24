#pragma once

#include "core/ManagerSet.h"
#include "core/TypeList.h"

namespace tanks
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
class ProfileManager;
class ResourceManager;
class StringsManager;
class UiStyle;
class UpgradeManager;
class AchievementManager;
class DifficultyManager;
class LevelLegendManager;

using PortAdapters = TypeList<view::cocos::CocosStorage, view::cocos::CocosAudioOutput, view::cocos::CocosKeyboardInput>;

using GameManagers = TypeList<model::SimulationClock, model::Scheduler, model::GameEvents, view::cocos::CocosClockDriver, ConfigReader, StringsManager, ProfileManager, ResourceManager, UiStyle, AudioManager, MusicManager, BuffCatalog, EnemyCatalog,
	AchievementManager, DifficultyManager, LevelLegendManager, UpgradeManager, ObjectsFactory, CommandsManager, InputManager, HoverManager, PlayerInputController>;

/**
 * Owns every game manager and their lifetime. The entry point owns the context; the platform layer calls
 * Initialize() once the engine runs and Shutdown() before the engine is torn down.
 * The ports (storage, audio output, input) are reached through the getters below.
 * Get() is the single global access point, for the View and code not yet migrated; the Model takes its
 * dependencies explicitly.
 */
class GameContext
{
public:
	GameContext();
	~GameContext();
	GameContext(const GameContext&) = delete;
	GameContext& operator=(const GameContext&) = delete;

	static GameContext& Get();

	void Initialize();
	void Shutdown();
	bool IsInitialized() const;

	ports::IStorage& GetStorage() const;
	ports::IAudioOutput& GetAudioOutput() const;
	ports::IInputSource& GetInputSource() const;

	template <typename TManager>
	TManager& GetManager() const
	{
		return Managers.Get<TManager>();
	}

private:
	ManagerSet<PortAdapters> Adapters;
	ManagerSet<GameManagers> Managers;
};
} // namespace tanks
