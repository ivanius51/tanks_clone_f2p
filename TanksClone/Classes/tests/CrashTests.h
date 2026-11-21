#pragma once

#include <string>
#include <vector>

#include "tests/TestFramework.h"

namespace tanks
{
namespace tests
{

// Debug and Test builds: the folder given by "--crash-folder=<path>", or empty for the default one.
std::string GetCrashFolderArgument();

// Debug and Test builds, "--crash-probe=<kind>": crashes on purpose one frame after launch. True if a probe was started.
bool StartCrashProbe();
bool IsCrashProbe();

// "--test=crash": starts the game once per crash kind with a probe, expects the crash exit code, a report whose
// stack names the probe function, and a minidump. These crashes are expected: they make the tests pass.
std::vector<TestResult> RunCrashTests();

} // namespace tests
} // namespace tanks
