#pragma once

#include <functional>
#include <vector>

#include "tests/TestFramework.h"

namespace tanks
{
namespace tests
{

// Plays real sounds through the engine: every output device, every file format in sounds/test,
// played at once and after a background preload, broken and missing files, freeing a sound while it plays,
// and the game's own path through AudioManager. Restores the profile's device at the end.
void RunAudioTests(const std::function<void(const std::vector<TestResult>&)>& InOnDone);

} // namespace tests
} // namespace tanks
