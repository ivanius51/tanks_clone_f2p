#include "model/GameEvents.h"

namespace tanks::model
{

void GameEvents::Emit(const std::string_view InName, const double InTime)
{
	Log.push_back({ std::string(InName), InTime });
	++Counts[std::string(InName)];
}

void GameEvents::Clear()
{
	Log.clear();
	Counts.clear();
}

bool GameEvents::Has(const std::string_view InName) const
{
	return GetCount(InName) > 0;
}

int GameEvents::GetCount(const std::string_view InName) const
{
	const auto Found = Counts.find(InName);
	return Found != Counts.end() ? Found->second : 0;
}

const std::vector<GameEvent>& GameEvents::GetLog() const
{
	return Log;
}

} // namespace tanks::model
