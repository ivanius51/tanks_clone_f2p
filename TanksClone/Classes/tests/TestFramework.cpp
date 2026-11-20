#include "tests/TestFramework.h"

#include <cstdio>
#include <string_view>

namespace tanks
{
namespace tests
{

void TestSuite::Add(const std::string& InName, const TestFunction& InTest)
{
	Tests.push_back({ InName, InTest });
}

size_t TestSuite::Size() const
{
	return Tests.size();
}

std::vector<TestResult> TestSuite::Run(cocos2d::Node* const InSandbox) const
{
	std::vector<TestResult> CurrentResults;
	for ( const auto& CurrentEntry : Tests )
	{
		TestContext CurrentContext(InSandbox);
		CurrentEntry.Test(CurrentContext);

		TestResult Result;
		Result.Name = CurrentEntry.Name;
		Result.Failures = CurrentContext.GetFailures();
		Result.Notes = CurrentContext.GetNotes();
		Result.bPassed = Result.Failures.empty();
		CurrentResults.push_back(Result);

		// Objects of the test leave the game here (their nodes exit the running scene).
		InSandbox->removeAllChildren();
	}
	return CurrentResults;
}

int Report(std::string_view InTitle, const std::vector<TestResult>& InResults, std::string& InReportText)
{
	int CurrentFailed = 0;
	std::ostringstream CurrentText;
	CurrentText << "==== " << InTitle << " ====\n";
	for ( const auto& Result : InResults )
	{
		CurrentText << (Result.bPassed ? "[PASS] " : "[FAIL] ") << Result.Name << "\n";
		for ( const auto& Failure : Result.Failures )
		{
			CurrentText << "         - " << Failure << "\n";
		}
		for ( const auto& CurrentNote : Result.Notes )
		{
			CurrentText << "         . " << CurrentNote << "\n";
		}
		if ( !Result.bPassed )
		{
			++CurrentFailed;
		}
	}
	CurrentText << InTitle << ": " << (InResults.size() - CurrentFailed) << " passed, " << CurrentFailed << " failed\n";

	InReportText += CurrentText.str();
	std::printf("%s", CurrentText.str().c_str());
	std::fflush(stdout);
	return CurrentFailed;
}

} // namespace tests
} // namespace tanks
