#include "model/GameEvents.h"

namespace tanks::model
{

void GameEvents::Emit(const std::string_view InName, const double InTime)
{
	const TStoredString Name(InName);
	Log.push_back({ Name, InTime });
	++Counts[Name];
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
	const auto Found = Counts.find(TStoredString::Find(InName));
	return Found != Counts.end() ? Found->second : 0;
}

const std::vector<GameEvent>& GameEvents::GetLog() const
{
	return Log;
}

} // namespace tanks::model
