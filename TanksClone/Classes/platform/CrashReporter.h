#pragma once

#include <string>

namespace tanks
{

/**
 * Local crash reporter. On a crash (a hardware exception, an uncaught C++ exception, std::terminate, a pure virtual
 * call or an invalid CRT parameter) it writes crash_<time>_<pid>.txt with the call stack and a .dmp minidump into the
 * crash folder, optionally tells the player where they are, and ends the process with CrashExitCode.
 * Windows only: elsewhere Install() does nothing.
 */
class CrashReporter
{
public:
	static constexpr int CrashExitCode = 2;

	// An empty InFolder means "crashes" next to the executable.
	static void Install(const std::string& InFolder, bool bInShowMessage);
	static const std::string& GetFolder();

	// The autotests: their results so far go first in the report, and the report is also written to InPath.
	static void SetPrefixText(const std::string* InText);
	static void SetExtraReportFile(const std::string& InPath);
};

} // namespace tanks
