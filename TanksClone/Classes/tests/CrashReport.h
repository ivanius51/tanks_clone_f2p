#pragma once

#include <string>

namespace tanks
{
namespace tests
{

/**
 * Turns a crash during the autotests into a report: the faulting address and the call stack are appended
 * to test_report.txt and the process exits with code 2 instead of the system's crash dialog. Windows only.
 */
class CrashReport
{
public:
	// InReportText is the text gathered so far; it is written first, so finished results are not lost.
	static void Install(const std::string* InReportText);
};

} // namespace tests
} // namespace tanks
