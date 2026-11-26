#include "tests/CrashTests.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string_view>

#include "cocos2d.h"
#include "core/BuildInfo.h"
#include "platform/CrashReporter.h"
#include "tests/TestRunner.h"
#include "templates/TContainers.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
#include <windows.h>
#endif

namespace game
{
namespace tests
{

namespace
{
	constexpr std::string_view ProbeArgument = "--crash-probe=";
	constexpr std::string_view FolderArgument = "--crash-folder=";
	constexpr unsigned long ProbeTimeoutMs = 60000;

	struct ProbeKind
	{
		std::string_view Name;
		std::string_view Function;
	};

	constexpr std::array<ProbeKind, 3> ProbeKinds = { {
		{ "access", "CrashProbeAccessViolation" },
		{ "exception", "CrashProbeUncaughtException" },
		{ "terminate", "CrashProbeTerminate" },
	} };

	// Each probe must stay a frame of its own, so the report can be checked for its name: no inlining, and no
	// optimization (a noreturn call such as std::terminate() would otherwise become a tail jump).
#pragma optimize("", off)
	__declspec(noinline) void CrashProbeAccessViolation()
	{
		int* volatile Target = nullptr;
		*Target = 1;
	}

	__declspec(noinline) void CrashProbeUncaughtException()
	{
		throw std::runtime_error("crash probe");
	}

	__declspec(noinline) void CrashProbeTerminate()
	{
		std::terminate();
	}
#pragma optimize("", on)

	std::string_view FindArgumentValue(const std::string_view InArgument)
	{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		const std::string_view CommandLine = GetCommandLineA();
		const std::size_t Start = CommandLine.find(InArgument);
		if ( Start == std::string_view::npos )
		{
			return {};
		}
		std::string_view Value = CommandLine.substr(Start + InArgument.size());
		if ( !Value.empty() && Value.front() == '"' )
		{
			Value.remove_prefix(1);
			return Value.substr(0, Value.find('"'));
		}
		return Value.substr(0, Value.find(' '));
#else
		return {};
#endif
	}

	std::string ReadFile(const std::filesystem::path& InPath)
	{
		std::ifstream Stream(InPath);
		return std::string(std::istreambuf_iterator<char>(Stream), std::istreambuf_iterator<char>());
	}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	// Runs this executable with a probe; returns its exit code, or -1 if it could not start or hung.
	int RunProbeProcess(const std::string_view InKind, const std::filesystem::path& InFolder)
	{
		std::array<wchar_t, MAX_PATH> ModulePath = {};
		GetModuleFileNameW(nullptr, ModulePath.data(), MAX_PATH);
		std::wstring CommandLine = L"\"" + std::wstring(ModulePath.data()) + L"\" --background --crash-probe="
			+ std::wstring(InKind.begin(), InKind.end()) + L" --crash-folder=\"" + InFolder.wstring() + L"\"";

		STARTUPINFOW Startup = {};
		Startup.cb = sizeof(Startup);
		PROCESS_INFORMATION Process = {};
		// CreateProcessW may write into the command line buffer, so it gets a mutable copy.
		if ( !CreateProcessW(nullptr, CommandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &Startup, &Process) )
		{
			return -1;
		}
		DWORD ExitCode = static_cast<DWORD>(-1);
		if ( WaitForSingleObject(Process.hProcess, ProbeTimeoutMs) == WAIT_OBJECT_0 )
		{
			GetExitCodeProcess(Process.hProcess, &ExitCode);
		}
		else
		{
			TerminateProcess(Process.hProcess, ExitCode);
		}
		CloseHandle(Process.hThread);
		CloseHandle(Process.hProcess);
		return static_cast<int>(ExitCode);
	}
#endif

	TestResult RunProbe(const ProbeKind& InKind)
	{
		TestResult Result;
		Result.Name = "Expected crash: " + std::string(InKind.Name) + " gives a report with its stack and a minidump";
		const auto Fail = [&Result](const std::string& InText)
		{
			Result.bPassed = false;
			Result.Failures.PushBack(InText);
		};
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		std::error_code Error;
		const std::filesystem::path Folder = std::filesystem::temp_directory_path(Error)
			/ ("TanksCloneCrash_" + std::to_string(GetCurrentProcessId()) + "_" + std::string(InKind.Name));
		std::filesystem::remove_all(Folder, Error);
		std::filesystem::create_directories(Folder, Error);

		const int ExitCode = RunProbeProcess(InKind.Name, Folder);
		if ( ExitCode != CrashReporter::CrashExitCode )
		{
			Fail("exit code " + std::to_string(ExitCode) + ", expected " + std::to_string(CrashReporter::CrashExitCode));
		}

		std::string Report;
		std::uintmax_t DumpSize = 0;
		for ( const auto& Entry : std::filesystem::directory_iterator(Folder, Error) )
		{
			if ( Entry.path().extension() == ".txt" )
			{
				Report = ReadFile(Entry.path());
			}
			else if ( Entry.path().extension() == ".dmp" )
			{
				DumpSize = Entry.file_size(Error);
			}
		}
		if ( Report.find("CRASH:") == std::string::npos )
		{
			Fail("no crash report in " + Folder.string());
		}
		else if ( Report.find(InKind.Function) == std::string::npos )
		{
			Fail("the stack does not name " + std::string(InKind.Function));
		}
		else
		{
			Result.Notes.PushBack("report: " + std::to_string(Report.size()) + " bytes, stack names " + std::string(InKind.Function));
		}
		if ( DumpSize == 0 )
		{
			Fail("no minidump");
		}
		std::filesystem::remove_all(Folder, Error);
#else
		Result.Notes.push_back("skipped: the crash reporter is Windows only");
#endif
		return Result;
	}
}

std::string GetCrashFolderArgument()
{
	return bHasDebugTools ? std::string(FindArgumentValue(FolderArgument)) : std::string();
}

bool IsCrashProbe()
{
	return bHasDebugTools && !FindArgumentValue(ProbeArgument).empty();
}

bool StartCrashProbe()
{
	if ( !IsCrashProbe() )
	{
		return false;
	}
	const std::string Kind(FindArgumentValue(ProbeArgument));
	cocos2d::Director::getInstance()->getScheduler()->schedule([Kind](float)
	{
		if ( Kind == "exception" )
		{
			CrashProbeUncaughtException();
		}
		else if ( Kind == "terminate" )
		{
			CrashProbeTerminate();
		}
		else
		{
			CrashProbeAccessViolation();
		}
	}, cocos2d::Director::getInstance(), 0.0f, 0, 0.0f, false, "CRASH_PROBE");
	return true;
}

TArray<TestResult> RunCrashTests()
{
	TArray<TestResult> Results;
	for ( const ProbeKind& Kind : ProbeKinds )
	{
		Results.PushBack(RunProbe(Kind));
	}
	return Results;
}

} // namespace tests
} // namespace game
