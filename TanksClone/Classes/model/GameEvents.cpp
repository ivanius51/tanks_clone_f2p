#include "model/GameEvents.h"

namespace game::model
{

void GameEvents::Emit(const std::string_view InName, const double InTime)
{
	const FTransientString Name(InName);
	Log.push_back({ Name, InTime });
	++Counts[Name];
}

void GameEvents::Clear()
{
	Log.clear();
	Counts.Clear();
}

bool GameEvents::Has(const std::string_view InName) const
{
	return GetCount(InName) > 0;
}

int GameEvents::GetCount(const std::string_view InName) const
{
	auto* const Found = Counts.Find(FTransientString::Find(InName));
	return Found ? *Found : 0;
}

const std::vector<GameEvent>& GameEvents::GetLog() const
{
	return Log;
}

} // namespace game::model
