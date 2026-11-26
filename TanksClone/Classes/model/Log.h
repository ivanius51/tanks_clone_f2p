#pragma once

namespace game::ports
{
class ILogSink;
}

namespace game::model
{

enum class ELogLevel
{
	Info,
	Warning,
	Error
};

/**
 * Engine-free logging for the Model and the code around it. Text goes to the installed sink (console, file, a
 * platform log); with no sink installed it is dropped. The sink is not owned and must outlive its use.
 */
class Log
{
public:
	static void SetSink(ports::ILogSink* InSink);

	static void Write(ELogLevel InLevel, const char* InFormat, ...);

	template <typename... TArgs>
	static void Info(const char* InFormat, TArgs... InArgs) { Write(ELogLevel::Info, InFormat, InArgs...); }

	template <typename... TArgs>
	static void Warn(const char* InFormat, TArgs... InArgs) { Write(ELogLevel::Warning, InFormat, InArgs...); }

	template <typename... TArgs>
	static void Error(const char* InFormat, TArgs... InArgs) { Write(ELogLevel::Error, InFormat, InArgs...); }
};

} // namespace game::model
