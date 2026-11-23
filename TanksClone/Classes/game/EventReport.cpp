#include "game/EventReport.h"

#include "core/GameContext.h"
#include "model/GameEvents.h"
#include "model/SimulationClock.h"

namespace tanks::events
{

void Report(const std::string_view InName)
{
	GameContext::Get().GetManager<model::GameEvents>().Emit(InName, GameContext::Get().GetManager<model::SimulationClock>().GetNow());
}

} // namespace tanks::events
