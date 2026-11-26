#include "platform/FileLogSink.h"

#include <cstdio>

#ifdef _WIN32
#include <windows.h>
#endif

namespace game
{

namespace
{
const char* LevelPrefix(const model::ELogLevel InLevel)
{
	switch ( InLevel )
	{
		case model::ELogLevel::Warning:
			return "[warning] ";
		case model::ELogLevel::Error:
			return "[error] ";
		default:
			return "";
	}
}
} // namespace

FileLogSink::FileLogSink(const std::string& InFilePath)
	: File(InFilePath, std::ios::out | std::ios::trunc)
{
}

void FileLogSink::Write(const model::ELogLevel InLevel, const std::string_view InText)
{
	const std::string Line = std::string(LevelPrefix(InLevel)) + std::string(InText) + "\n";
	std::fputs(Line.c_str(), stdout);
	std::fflush(stdout);
#ifdef _WIN32
	OutputDebugStringA(Line.c_str());
#endif
	if ( File.is_open() )
	{
		File << Line;
		File.flush();
	}
}

} // namespace game
