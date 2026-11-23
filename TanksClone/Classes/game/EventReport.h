#pragma once

#include <string_view>

namespace tanks::events
{

// Reports a named event of the current level (model::GameEvents) with the current game time.
void Report(std::string_view InName);

} // namespace tanks::events
