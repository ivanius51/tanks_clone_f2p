#include "tests/CrashReport.h"

#include <array>
#include <cstdio>
#include <fstream>

#include "platform/CCPlatformConfig.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
#include <windows.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")
#endif

namespace tanks
{
namespace tests
{

namespace
{
	const std::string* GReportText = nullptr;
	constexpr int CrashExitCode = 2;
	constexpr int MaxFrames = 48;

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	std::string DescribeAddress(HANDLE InProcess, const DWORD64 InAddress)
	{
		std::array<char, sizeof(SYMBOL_INFO) + MAX_SYM_NAME> SymbolBuffer = {};
		auto Symbol = reinterpret_cast<SYMBOL_INFO*>(SymbolBuffer.data());
		Symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		Symbol->MaxNameLen = MAX_SYM_NAME;

		std::array<char, 512> Line = {};
		IMAGEHLP_MODULE64 Module = {};
		Module.SizeOfStruct = sizeof(Module);
		const char* ModuleName = SymGetModuleInfo64(InProcess, InAddress, &Module) ? Module.ModuleName : "?";
		DWORD64 Offset = 0;
		if ( SymFromAddr(InProcess, InAddress, &Offset, Symbol) )
		{
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
		}
		else
		{
			std::snprintf(Line.data(), Line.size(), "  %s!0x%llx\n", ModuleName, InAddress);
		}
		return Line.data();
	}

	// StackWalk64 needs a copy of the context: it updates the registers while it walks.
	std::string DescribeStack(EXCEPTION_POINTERS* const InException)
	{
		HANDLE Process = GetCurrentProcess();
		SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
		SymInitialize(Process, nullptr, TRUE);

		CONTEXT Context = *InException->ContextRecord;
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

	LONG WINAPI OnCrash(EXCEPTION_POINTERS* const InException)
	{
		std::array<char, 128> Header = {};
		std::snprintf(Header.data(), Header.size(), "\nCRASH: exception 0x%08lx at 0x%p\n",
			InException->ExceptionRecord->ExceptionCode, InException->ExceptionRecord->ExceptionAddress);
		const std::string Text = (GReportText ? *GReportText : std::string()) + Header.data() + DescribeStack(InException) + "\nTOTAL: FAILED (crash)\n";
		std::fputs(Text.c_str(), stdout);
		std::fflush(stdout);
		std::ofstream("test_report.txt") << Text;
		// TerminateProcess skips DLL detach: the engine is in an unknown state after the crash.
		TerminateProcess(GetCurrentProcess(), CrashExitCode);
		return EXCEPTION_EXECUTE_HANDLER;
	}
#endif
}

void CrashReport::Install(const std::string* const InReportText)
{
	GReportText = InReportText;
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	SetUnhandledExceptionFilter(OnCrash);
#endif
}

} // namespace tests
} // namespace tanks
