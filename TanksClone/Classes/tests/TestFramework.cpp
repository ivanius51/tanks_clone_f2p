#include "tests/TestFramework.h"
#include "templates/TContainers.h"

#include <cstdio>

namespace game
{
namespace tests
{

void TestSuite::Add(const std::string& InName, const TestFunction& InTest)
{
	Tests.PushBack({ InName, InTest });
}

size_t TestSuite::Size() const
{
	return Tests.Size();
}

TArray<TestResult> TestSuite::Run(cocos2d::Node* const InSandbox) const
{
	TArray<TestResult> CurrentResults;
	for ( const auto& CurrentEntry : Tests )
	{
		TestContext CurrentContext(InSandbox);
		CurrentEntry.Test(CurrentContext);

		TestResult Result;
		Result.Name = CurrentEntry.Name;
		Result.Failures = CurrentContext.GetFailures();
		Result.Notes = CurrentContext.GetNotes();
		Result.bPassed = Result.Failures.IsEmpty();
		CurrentResults.PushBack(Result);

		// Objects of the test leave the game here (their nodes exit the running scene).
		InSandbox->removeAllChildren();
	}
	return CurrentResults;
}

int Report(std::string_view InTitle, const TArray<TestResult>& InResults, std::string& InReportText)
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
	CurrentText << InTitle << ": " << (InResults.Size() - CurrentFailed) << " passed, " << CurrentFailed << " failed\n";

	InReportText += CurrentText.str();
	std::printf("%s", CurrentText.str().c_str());
	std::fflush(stdout);
	return CurrentFailed;
}

} // namespace tests
} // namespace game
