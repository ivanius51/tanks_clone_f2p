#pragma once

#include <chrono>
#include <string>

#include "core/BuildInfo.h"

namespace game::model
{
/**
 * Collects the time spent in named code sections (call count, total, worst) from any thread, for hunting slow
 * spots in Debug and Test builds. Release builds record nothing. Read the result with BuildReport().
 */
class Profiler
{
public:
	static void Record(const char* InName, std::chrono::nanoseconds InElapsed);
	static void Reset();
	/** One line per section, the most expensive first. */
	static std::string BuildReport();
};

/** Times the rest of the enclosing block under InName (a string literal) and records it on exit. */
class ProfileScope
{
public:
	explicit ProfileScope(const char* InName)
	{
		if constexpr ( bHasDebugTools )
		{
			Name = InName;
			Start = std::chrono::steady_clock::now();
		}
	}

	~ProfileScope()
	{
		if constexpr ( bHasDebugTools )
		{
			Profiler::Record(Name, std::chrono::steady_clock::now() - Start);
		}
	}

	ProfileScope(const ProfileScope&) = delete;
	ProfileScope& operator=(const ProfileScope&) = delete;

private:
	const char* Name = nullptr;
	std::chrono::steady_clock::time_point Start;
};
} // namespace game::model
