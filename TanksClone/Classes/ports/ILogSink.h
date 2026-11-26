#pragma once

#include <string_view>

#include "model/Log.h"

namespace game::ports
{
/**
 * Port: takes finished log lines from model::Log. Where they go (console, debugger, a file next to the crash
 * reports, the browser console) is the adapter's business.
 */
class ILogSink
{
public:
	virtual ~ILogSink() = default;

	virtual void Write(model::ELogLevel InLevel, std::string_view InText) = 0;
};
} // namespace game::ports
