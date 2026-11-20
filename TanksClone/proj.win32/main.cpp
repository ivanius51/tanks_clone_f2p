#include "main.h"
#include "AppDelegate.h"
#include "cocos2d.h"
#include "core/GameContext.h"

USING_NS_CC;

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	// Declared first, so the managers outlive the application object.
	tanks::GameContext Context;
	AppDelegate app;
	return Application::getInstance()->run();
}
