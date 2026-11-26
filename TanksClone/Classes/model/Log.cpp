#include "model/Log.h"

#include <cstdarg>
#include <cstdio>
#include <string>

#include "ports/ILogSink.h"

namespace game::model
{

namespace
{
ports::ILogSink* GSink{ nullptr };
}

void Log::SetSink(ports::ILogSink* const InSink)
{
	GSink = InSink;
}

void Log::Write(const ELogLevel InLevel, const char* const InFormat, ...)
{
	if ( !GSink )
	{
		return;
	}

	va_list Args;
	va_start(Args, InFormat);
	va_list Measure;
	va_copy(Measure, Args);
	const int Length = std::vsnprintf(nullptr, 0, InFormat, Measure);
	va_end(Measure);
	if ( Length < 0 )
	{
		va_end(Args);
		return;
	}

	std::string Text(static_cast<size_t>(Length), '\0');
	std::vsnprintf(Text.data(), Text.size() + 1, InFormat, Args);
	va_end(Args);
	GSink->Write(InLevel, Text);
}

} // namespace game::model
