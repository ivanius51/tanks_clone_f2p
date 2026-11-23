#include "tests/TestSelection.h"

namespace tanks
{
namespace tests
{

namespace
{
	constexpr std::string_view TestArgument = "--test";

	TestSelection Short()
	{
		TestSelection Result;
		Result.bUnit = true;
		Result.bFields = true;
		return Result;
	}

	std::string_view FindTestArgument(const std::string_view InCommandLine)
	{
		size_t Start = 0;
		while ( Start < InCommandLine.size() )
		{
			const size_t End = InCommandLine.find(' ', Start);
			const std::string_view Token = InCommandLine.substr(Start, End == std::string_view::npos ? std::string_view::npos : End - Start);
			if ( Token == TestArgument || (Token.size() > TestArgument.size() && Token.substr(0, TestArgument.size() + 1) == "--test=") )
			{
				return Token;
			}
			if ( End == std::string_view::npos )
			{
				break;
			}
			Start = End + 1;
		}
		return {};
	}
}

TestSelection TestSelection::Parse(const std::string_view InCommandLine)
{
	const std::string_view Argument = FindTestArgument(InCommandLine);
	if ( Argument.empty() )
	{
		return TestSelection();
	}
	if ( Argument == TestArgument )
	{
		return Short();
	}

	const std::string_view Mode = Argument.substr(TestArgument.size() + 1);
	TestSelection Result;
	if ( Mode == "short" )
	{
		Result = Short();
	}
	else if ( Mode == "unit" )
	{
		Result.bUnit = true;
	}
	else if ( Mode == "menu" )
	{
		Result.bMenu = true;
	}
	else if ( Mode == "benchmark" )
	{
		Result.bBenchmark = true;
	}
	else if ( Mode == "crash" )
	{
		Result.bCrash = true;
	}
	else if ( Mode == "editor" )
	{
		Result.bEditor = true;
	}
	else if ( Mode == "audio" )
	{
		Result.bAudio = true;
	}
	else if ( Mode == "campaign" )
	{
		Result.bCampaign = true;
	}
	else if ( Mode == "full" )
	{
		Result = Short();
		Result.bCampaign = true;
		Result.bBenchmark = true;
		Result.bMenu = true;
		Result.bEditor = true;
		Result.bCrash = true;
		Result.bAudio = true;
	}
	else
	{
		Result = Short();
		Result.bValid = false;
	}
	return Result;
}

bool TestSelection::IsEmpty() const
{
	return !bUnit && !HasLevelCases() && !bMenu && !bEditor && !bCrash && !bAudio;
}

bool TestSelection::HasLevelCases() const
{
	return bFields || bCampaign || bBenchmark;
}

} // namespace tests
} // namespace tanks
