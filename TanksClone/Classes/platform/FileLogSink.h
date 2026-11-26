#pragma once

#include <fstream>
#include <string>

#include "ports/ILogSink.h"

namespace game
{

/**
 * Log sink for desktop builds: every line goes to stdout, the debugger output (Windows) and a log file that is
 * restarted on each run and flushed per line, so it survives a crash and sits next to the crash reports.
 */
class FileLogSink : public ports::ILogSink
{
public:
	explicit FileLogSink(const std::string& InFilePath);

	void Write(model::ELogLevel InLevel, std::string_view InText) override;

private:
	std::ofstream File;
};

} // namespace game
