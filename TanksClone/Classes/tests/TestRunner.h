#pragma once

#include <string_view>

namespace tanks
{
namespace tests
{

// Runs all automatic tests and then ends the process (exit code 0 = everything passed):
//  1. unit tests;
//  2. level tests: the bot plays the test fields, the campaign levels and the crowd benchmark in fast forward;
//  3. menu tests: the screens are walked through by pressing their buttons.
// Which of them run is chosen by the --test mode (see TestSelection).
// The profile and the custom level go to a temporary folder: the player's own files are not touched.
// The report goes to the console and to test_report.txt. Start with "TanksClone.exe --test".
// Call it instead of showing the first scene.
void RunAutomaticTests();

// Points the writable folder (profile, custom level) to a fresh temporary one.
void UseTemporaryProfile();

// True if the program was started in test mode (--test on the command line, or TANKS_TEST in the environment).
// Always false in the Release build (see bHasDebugTools).
bool IsTestMode();

// True if the exact text InArgument appears on the command line (always false outside Windows).
bool HasCommandLineArgument(std::string_view InArgument);

// "--background" (Debug and Test builds): the window opens behind the others and gives the focus back.
bool IsBackgroundMode();

} // namespace tests
} // namespace tanks
