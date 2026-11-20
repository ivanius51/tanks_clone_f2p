#pragma once

#include <string_view>

namespace tanks
{
namespace tests
{

/**
 * Which autotest modules a run includes, read from the command line:
 * "--test" (short: unit tests and the small test fields), "--test=menu", "--test=benchmark" (the crowd level),
 * "--test=campaign" (the bot plays every built-in level) and "--test=full" (all of them).
 */
struct TestSelection
{
	bool bUnit = false;
	bool bFields = false;
	bool bCampaign = false;
	bool bBenchmark = false;
	bool bMenu = false;
	bool bValid = true;

	// Without a "--test" argument the selection is empty; an unknown mode gives the short run with bValid = false.
	static TestSelection Parse(std::string_view InCommandLine);

	bool IsEmpty() const;
	bool HasLevelCases() const;
};

} // namespace tests
} // namespace tanks
