#include "game/EventReport.h"

#include "core/GameContext.h"
#include "model/GameEvents.h"
#include "model/SimulationClock.h"

namespace game::events
{

void Report(const std::string_view InName)
{
	GameContext::Get<model::GameEvents>().Emit(InName, GameContext::Get<model::SimulationClock>().GetNow());
}

} // namespace game::events
