#pragma once

#include "core/ManagerSet.h"
#include "core/TypeList.h"

namespace tanks
{

class AudioManager;
class BuffCatalog;
class CommandsManager;
class ConfigReader;
class EnemyCatalog;
class KeyboardController;
class ObjectsFactory;
class ProfileManager;
class ResourceManager;
class StringsManager;
class UpgradeManager;

// Creation order: a manager may use only the ones listed before it; they are destroyed in reverse.
using GameManagers = TypeList<ConfigReader, StringsManager, ProfileManager, ResourceManager, AudioManager, BuffCatalog, EnemyCatalog,
	UpgradeManager, ObjectsFactory, CommandsManager, KeyboardController>;

/**
 * Owns every game manager and their lifetime. The entry point owns the context; the platform layer calls
 * Initialize() once the engine runs and Shutdown() before the engine is torn down.
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

	template <typename TManager>
	TManager& GetManager() const
	{
		return Managers.Get<TManager>();
	}

private:
	ManagerSet<GameManagers> Managers;
};

} // namespace tanks
