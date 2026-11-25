#include "platform/CrashReporter.h"

#include <array>
#include <cstdio>
#include <exception>
#include <fstream>

#include "platform/CCPlatformConfig.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
#include <windows.h>
#include <dbghelp.h>
#include <stdlib.h>
#pragma comment(lib, "dbghelp.lib")
#endif

namespace game
{

namespace
{
	struct CrashState
	{
		std::string Folder;
		std::string ExtraReportFile;
		const std::string* PrefixText = nullptr;
		bool bShowMessage = false;
		bool bReporting = false;
	};

	CrashState GState;

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	constexpr int MaxFrames = 64;

	std::wstring ToWide(const std::string& InText)
	{
		const int Count = MultiByteToWideChar(CP_UTF8, 0, InText.c_str(), -1, nullptr, 0);
		std::wstring Result(Count > 0 ? Count - 1 : 0, L'\0');
		MultiByteToWideChar(CP_UTF8, 0, InText.c_str(), -1, Result.data(), Count);
		return Result;
	}

	std::string GetDefaultFolder()
	{
		std::array<wchar_t, MAX_PATH> ModulePath = {};
		const DWORD Length = GetModuleFileNameW(nullptr, ModulePath.data(), MAX_PATH);
		std::wstring Folder(ModulePath.data(), Length);
		Folder = Folder.substr(0, Folder.find_last_of(L"\\/") + 1) + L"crashes\\";
		const int Count = WideCharToMultiByte(CP_UTF8, 0, Folder.c_str(), -1, nullptr, 0, nullptr, nullptr);
		std::string Result(Count > 0 ? Count - 1 : 0, '\0');
		WideCharToMultiByte(CP_UTF8, 0, Folder.c_str(), -1, Result.data(), Count, nullptr, nullptr);
		return Result;
	}

	std::string DescribeAddress(const HANDLE InProcess, const DWORD64 InAddress)
	{
		std::array<char, sizeof(SYMBOL_INFO) + MAX_SYM_NAME> SymbolBuffer = {};
		auto Symbol = reinterpret_cast<SYMBOL_INFO*>(SymbolBuffer.data());
		Symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		Symbol->MaxNameLen = MAX_SYM_NAME;

		std::array<char, 768> Line = {};
		IMAGEHLP_MODULE64 Module = {};
		Module.SizeOfStruct = sizeof(Module);
		const char* ModuleName = SymGetModuleInfo64(InProcess, InAddress, &Module) ? Module.ModuleName : "?";
		DWORD64 Offset = 0;
		if ( !SymFromAddr(InProcess, InAddress, &Offset, Symbol) )
		{
			std::snprintf(Line.data(), Line.size(), "  %s!0x%llx\n", ModuleName, InAddress);
			return Line.data();
		}
		IMAGEHLP_LINE64 SourceLine = {};
		SourceLine.SizeOfStruct = sizeof(SourceLine);
		DWORD LineOffset = 0;
		if ( SymGetLineFromAddr64(InProcess, InAddress, &LineOffset, &SourceLine) )
		{
			std::snprintf(Line.data(), Line.size(), "  %s!%s+0x%llx (%s:%lu)\n", ModuleName, Symbol->Name, Offset, SourceLine.FileName, SourceLine.LineNumber);
		}
		else
		{
			std::snprintf(Line.data(), Line.size(), "  %s!%s+0x%llx\n", ModuleName, Symbol->Name, Offset);
		}
		return Line.data();
	}

	// StackWalk64 needs its own copy of the context: it updates the registers while it walks (32-bit build).
	std::string DescribeStack(const CONTEXT& InContext)
	{
		const HANDLE Process = GetCurrentProcess();
		SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
		SymInitialize(Process, nullptr, TRUE);

		CONTEXT Context = InContext;
		STACKFRAME64 Frame = {};
		Frame.AddrPC.Offset = Context.Eip;
		Frame.AddrFrame.Offset = Context.Ebp;
		Frame.AddrStack.Offset = Context.Esp;
		Frame.AddrPC.Mode = Frame.AddrFrame.Mode = Frame.AddrStack.Mode = AddrModeFlat;

		std::string Result;
		for ( int Index = 0; Index < MaxFrames; ++Index )
		{
			if ( !StackWalk64(IMAGE_FILE_MACHINE_I386, Process, GetCurrentThread(), &Frame, &Context, nullptr,
				SymFunctionTableAccess64, SymGetModuleBase64, nullptr) || Frame.AddrPC.Offset == 0 )
			{
				break;
			}
			Result += DescribeAddress(Process, Frame.AddrPC.Offset);
		}
		return Result;
	}

	void WriteMinidump(const std::string& InPath, EXCEPTION_POINTERS* const InException)
	{
		const HANDLE File = CreateFileW(ToWide(InPath).c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if ( File == INVALID_HANDLE_VALUE )
		{
			return;
		}
		MINIDUMP_EXCEPTION_INFORMATION Exception = {};
		Exception.ThreadId = GetCurrentThreadId();
		Exception.ExceptionPointers = InException;
		Exception.ClientPointers = FALSE;
		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), File,
			static_cast<MINIDUMP_TYPE>(MiniDumpWithThreadInfo | MiniDumpWithIndirectlyReferencedMemory), &Exception, nullptr, nullptr);
		CloseHandle(File);
	}

	std::string MakeBaseName()
	{
		SYSTEMTIME Time = {};
		GetLocalTime(&Time);
		std::array<char, 64> Name = {};
		std::snprintf(Name.data(), Name.size(), "crash_%04u%02u%02u_%02u%02u%02u_%lu", Time.wYear, Time.wMonth, Time.wDay,
			Time.wHour, Time.wMinute, Time.wSecond, GetCurrentProcessId());
		return Name.data();
	}

	[[noreturn]] void ReportAndExit(const std::string& InReason, EXCEPTION_POINTERS* const InException)
	{
		if ( GState.bReporting )
		{
			TerminateProcess(GetCurrentProcess(), CrashReporter::CrashExitCode);
		}
		GState.bReporting = true;

		const std::string Report = "\nCRASH: " + InReason + "\n" + DescribeStack(*InException->ContextRecord);
		std::fputs(Report.c_str(), stdout);
		std::fflush(stdout);

		CreateDirectoryW(ToWide(GState.Folder).c_str(), nullptr);
		const std::string BasePath = GState.Folder + MakeBaseName();
		std::ofstream(ToWide(BasePath + ".txt")) << Report;
		WriteMinidump(BasePath + ".dmp", InException);
		if ( !GState.ExtraReportFile.empty() )
		{
			std::ofstream(GState.ExtraReportFile) << (GState.PrefixText ? *GState.PrefixText : std::string()) << Report << "\nTOTAL: FAILED (crash)\n";
		}
		if ( GState.bShowMessage )
		{
			const std::wstring Message = L"The game crashed. A report was saved to:\n" + ToWide(BasePath) + L".txt";
			MessageBoxW(nullptr, Message.c_str(), L"TanksClone", MB_OK | MB_ICONERROR | MB_TOPMOST);
		}
		// TerminateProcess skips DLL detach: the engine is in an unknown state after the crash.
		TerminateProcess(GetCurrentProcess(), CrashReporter::CrashExitCode);
		for ( ;; )
		{
		}
	}

	// Crashes found outside an SEH exception have no exception record: the current context stands in for one.
	[[noreturn]] void ReportHere(const std::string& InReason)
	{
		CONTEXT Context = {};
		RtlCaptureContext(&Context);
		EXCEPTION_RECORD Record = {};
		Record.ExceptionCode = STATUS_FATAL_APP_EXIT;
		Record.ExceptionAddress = reinterpret_cast<PVOID>(Context.Eip);
		EXCEPTION_POINTERS Pointers = { &Record, &Context };
		ReportAndExit(InReason, &Pointers);
	}

	LONG WINAPI OnUnhandledException(EXCEPTION_POINTERS* const InException)
	{
		std::array<char, 96> Reason = {};
		std::snprintf(Reason.data(), Reason.size(), "exception 0x%08lx at 0x%p",
			InException->ExceptionRecord->ExceptionCode, InException->ExceptionRecord->ExceptionAddress);
		ReportAndExit(Reason.data(), InException);
	}

	void OnTerminate()
	{
		ReportHere("std::terminate");
	}

	void OnPureCall()
	{
		ReportHere("pure virtual function call");
	}

	void OnInvalidParameter(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t)
	{
		ReportHere("invalid parameter passed to a CRT function");
	}
#endif
}

void CrashReporter::Install(const std::string& InFolder, const bool bInShowMessage)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	GState.Folder = InFolder.empty() ? GetDefaultFolder() : InFolder;
	if ( GState.Folder.back() != '\\' && GState.Folder.back() != '/' )
	{
		GState.Folder += '\\';
	}
	GState.bShowMessage = bInShowMessage;
	SetUnhandledExceptionFilter(OnUnhandledException);
	std::set_terminate(OnTerminate);
	_set_purecall_handler(OnPureCall);
	_set_invalid_parameter_handler(OnInvalidParameter);
	// abort() otherwise shows its own dialog and skips the report.
	_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif
}

const std::string& CrashReporter::GetFolder()
{
	return GState.Folder;
}

void CrashReporter::SetPrefixText(const std::string* const InText)
{
	GState.PrefixText = InText;
}

void CrashReporter::SetExtraReportFile(const std::string& InPath)
{
	GState.ExtraReportFile = InPath;
}

} // namespace game
