#include "core/GameContext.h"

#include <cassert>

#include "core/ConfigReader.h"
#include "game/CommandsManager.h"
#include "game/components/Buff.h"
#include "game/EnemyCatalog.h"
#include "game/KeyboardController.h"
#include "game/ObjectsFactory.h"
#include "services/AudioManager.h"
#include "services/ProfileManager.h"
#include "services/ResourceManager.h"
#include "services/StringsManager.h"
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
	Managers.Create();
}

void GameContext::Shutdown()
{
	Managers.Destroy();
}

bool GameContext::IsInitialized() const
{
	return Managers.IsCreated();
}

} // namespace tanks
