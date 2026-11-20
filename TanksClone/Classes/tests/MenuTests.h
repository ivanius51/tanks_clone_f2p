#pragma once

#include <functional>
#include <vector>
#include "tests/TestFramework.h"

namespace tanks
{
namespace tests
{

// Walks through the screens the way a player would, by pressing buttons and keys:
// menu, settings (language, sound, FPS), upgrades (no coins, a purchase), a level (pause, resume,
// result screen, back to the menu), the editor and the custom level. Every step waits for the
// expected screen. Calls InOnDone with one result per step. Uses the profile in the writable folder,
// so the test runner must point that folder somewhere temporary first.
void RunMenuTests(const std::function<void(const std::vector<TestResult>&)>& InOnDone);

} // namespace tests
} // namespace tanks
