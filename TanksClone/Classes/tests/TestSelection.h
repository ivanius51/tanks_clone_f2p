#pragma once

#include <string_view>

namespace tanks
{
namespace tests
{

/**
 * Which autotest modules a run includes, read from the command line:
 * "--test" (short: unit tests and the small test fields), "--test=unit" (unit tests only), "--test=menu", "--test=benchmark" (the crowd level),
 * "--test=campaign" (the bot plays every built-in level), "--test=crash" (expected crashes give reports) and "--test=full" (all of them).
 */
struct TestSelection
{
	bool bUnit = false;
	bool bFields = false;
	bool bCampaign = false;
	bool bBenchmark = false;
	bool bMenu = false;
	bool bCrash = false;
	bool bAudio = false;
	bool bValid = true;

	// Without a "--test" argument the selection is empty; an unknown mode gives the short run with bValid = false.
	static TestSelection Parse(std::string_view InCommandLine);

	bool IsEmpty() const;
	bool HasLevelCases() const;
};

} // namespace tests
} // namespace tanks
