#pragma once


#include "templates/TFunction.h"
#include "tests/TestFramework.h"
#include "templates/TContainers.h"

namespace game
{
namespace tests
{

// Plays real sounds through the engine: every output device, every file format in sounds/test,
// played at once and after a background preload, broken and missing files, freeing a sound while it plays,
// and the game's own path through AudioManager. Restores the profile's device at the end.
void RunAudioTests(const TFunction<void(const TArray<TestResult>&)>& InOnDone);

} // namespace tests
} // namespace game
