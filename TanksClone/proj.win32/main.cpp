#include "main.h"
#include "AppDelegate.h"
#include "cocos2d.h"
#include "core/GameContext.h"
#include "platform/CrashReporter.h"
#include "tests/CrashTests.h"
#include "tests/TestRunner.h"

USING_NS_CC;

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	const bool bAutomatic = game::tests::IsTestMode() || game::tests::IsCrashProbe() || game::tests::IsBackgroundMode();
	game::CrashReporter::Install(game::tests::GetCrashFolderArgument(), !bAutomatic);

	// Declared first, so the managers outlive the application object.
	game::GameContext Context;
	AppDelegate app;
	return Application::getInstance()->run();
}
