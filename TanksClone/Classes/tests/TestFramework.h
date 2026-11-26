#pragma once

#include <sstream>
#include <string>
#include <string_view>

#include "cocos2d.h"

#include "templates/TFunction.h"
#include "templates/TContainers.h"
#include "templates/TStoredString.h"

template <bool bPersistent>
std::ostream& operator<<(std::ostream& InStream, const TStoredString<bPersistent> InText)
{
	return InStream << InText.View();
}

namespace game
{
namespace tests
{

// What a test uses to check things and to get a place to put game objects.
class TestContext
{
public:
	explicit TestContext(cocos2d::Node* const InSandbox)
		: Sandbox(InSandbox)
	{
	}

	// A running node that tests may add objects to; it is emptied after every test.
	cocos2d::Node* GetSandbox() const { return Sandbox; }

	void Expect(const bool bInCondition, const std::string& InWhat)
	{
		if ( !bInCondition )
		{
			Failures.PushBack(InWhat);
		}
	}

	template <typename A, typename B>
	void ExpectEqual(const A& InActual, const B& InExpected, std::string_view InWhat)
	{
		if ( !(InActual == InExpected) )
		{
			std::ostringstream Message;
			Message << InWhat << ": expected " << InExpected << ", got " << InActual;
			Failures.PushBack(Message.str());
		}
	}

	void ExpectNear(const float InActual, const float InExpected, const float InTolerance, std::string_view InWhat)
	{
		if ( InActual < InExpected - InTolerance || InActual > InExpected + InTolerance )
		{
			std::ostringstream Message;
			Message << InWhat << ": expected " << InExpected << " +-" << InTolerance << ", got " << InActual;
			Failures.PushBack(Message.str());
		}
	}

	// Extra information shown in the report even when the test passes.
	void Note(std::string_view InText) { Notes.EmplaceBack(InText); }

	const TArray<std::string>& GetFailures() const { return Failures; }
	const TArray<std::string>& GetNotes() const { return Notes; }

private:
	cocos2d::Node* Sandbox;
	TArray<std::string> Failures;
	TArray<std::string> Notes;
};

struct TestResult
{
	std::string Name;
	bool bPassed = true;
	TArray<std::string> Failures;
	TArray<std::string> Notes;
};

typedef TFunction<void(TestContext&), UiCallbackInlineSize> TestFunction;

// A list of named tests.
class TestSuite
{
public:
	void Add(const std::string& InName, const TestFunction& InTest);
	// Runs every test (objects they create go to InSandbox, which is emptied after each one).
	TArray<TestResult> Run(cocos2d::Node* InSandbox) const;
	size_t Size() const;

private:
	struct Entry
	{
		std::string Name;
		TestFunction Test;
	};
	TArray<Entry> Tests;
};

// Prints the results and writes them to a file. Returns the number of failed tests.
int Report(std::string_view InTitle, const TArray<TestResult>& InResults, std::string& InReportText);

} // namespace tests
} // namespace game
