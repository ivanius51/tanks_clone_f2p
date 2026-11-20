#include "AppDelegate.h"

#include <array>
#include <string>

#include "core/BuildInfo.h"
#include "core/GameConstants.h"
#include "game/CommandsManager.h"
#include "game/KeyboardController.h"
#include "game/ObjectsFactory.h"
#include "scenes/LoadingScene.h"
#include "services/AudioManager.h"
#include "services/DisplaySettings.h"
#include "tests/Showcase.h"
#include "tests/TestRunner.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
#include <mmsystem.h>
#endif

namespace
{
	const cocos2d::Size DesignResolutionSize(tanks::DesignWidth, tanks::DesignHeight);

	constexpr const char* WindowTitle = "TanksClone";
	constexpr const char* ResourcesFolder = "Resources";
	constexpr unsigned TimerResolutionMs = 1;

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	std::string GetExecutableFolder()
	{
		std::array<wchar_t, MAX_PATH> ModulePath = {};
		const DWORD Length = GetModuleFileNameW(nullptr, ModulePath.data(), MAX_PATH);
		std::wstring Folder(ModulePath.data(), Length);
		Folder = Folder.substr(0, Folder.find_last_of(L"\\/") + 1);

		// FileUtils works with UTF-8 paths; the folder may contain non-ASCII characters.
		const int ByteCount = WideCharToMultiByte(CP_UTF8, 0, Folder.c_str(), -1, nullptr, 0, nullptr, nullptr);
		std::string Result(ByteCount > 0 ? ByteCount - 1 : 0, '\0');
		WideCharToMultiByte(CP_UTF8, 0, Folder.c_str(), -1, &Result[0], ByteCount, nullptr, nullptr);
		return Result;
	}
#endif

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	HWND GPreviousForegroundWindow = nullptr;

	// Not minimized: a minimized window stops the engine's main loop, and the tests need it running.
	void SendWindowToBackground(cocos2d::GLView* const InView)
	{
		const HWND GameWindow = InView->getWin32Window();
		SetWindowPos(GameWindow, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
		if ( GPreviousForegroundWindow && GPreviousForegroundWindow != GameWindow )
		{
			// Allowed only while this process owns the foreground, i.e. right after the window was created.
			SetForegroundWindow(GPreviousForegroundWindow);
		}
	}
#endif

	void AddDebugStatsToggle()
	{
		auto Keyboard = cocos2d::EventListenerKeyboard::create();
		Keyboard->onKeyPressed = [](const cocos2d::EventKeyboard::KeyCode InKeyCode, cocos2d::Event*)
		{
			if ( InKeyCode == cocos2d::EventKeyboard::KeyCode::KEY_F3 )
			{
				auto DirectorInstance = cocos2d::Director::getInstance();
				DirectorInstance->setDisplayStats(!DirectorInstance->isDisplayStats());
			}
		};
		cocos2d::Director::getInstance()->getEventDispatcher()->addEventListenerWithFixedPriority(Keyboard, 1);
	}
}

AppDelegate::AppDelegate()
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	// The game finds its files next to the executable, wherever it is started from.
	cocos2d::FileUtils::getInstance()->setDefaultResourceRootPath(GetExecutableFolder() + ResourcesFolder + "\\");
	// The main loop waits between frames with Sleep(); the default 15.6 ms timer makes every limit above ~60 FPS
	// and every limit between the timer steps inexact.
	timeBeginPeriod(TimerResolutionMs);
	GPreviousForegroundWindow = GetForegroundWindow();
#endif
}

AppDelegate::~AppDelegate()
{
	tanks::AudioManager::GetInstance().Shutdown();
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	timeEndPeriod(TimerResolutionMs);
#endif
}

void AppDelegate::initGLContextAttrs()
{
	// red, green, blue, alpha, depth, stencil, multisamples
	GLContextAttrs GlContextAttrs = { 8, 8, 8, 8, 24, 8, 0 };
	cocos2d::GLView::setGLContextAttrs(GlContextAttrs);
}

bool AppDelegate::applicationDidFinishLaunching()
{
	auto DirectorInstance = cocos2d::Director::getInstance();
	auto Glview = DirectorInstance->getOpenGLView();
	if ( !Glview )
	{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) || (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
		Glview = cocos2d::GLViewImpl::createWithRect(WindowTitle, cocos2d::Rect(0, 0, DesignResolutionSize.width, DesignResolutionSize.height));
#else
		glview = GLViewImpl::create(WINDOW_TITLE);
#endif
		DirectorInstance->setOpenGLView(Glview);
	}
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	if ( tanks::tests::IsBackgroundMode() )
	{
		SendWindowToBackground(Glview);
	}
#endif

	// The engine's stats (FPS, draw calls) cover the player's panel: hidden, F3 shows them in Debug and Test builds.
	DirectorInstance->setDisplayStats(false);
	if ( tanks::bHasDebugTools )
	{
		AddDebugStatsToggle();
	}

	tanks::ApplyMenuFrameRate();
	Glview->setDesignResolutionSize(DesignResolutionSize.width, DesignResolutionSize.height, ResolutionPolicy::SHOW_ALL);
	// The content scale follows the texture set (HD or SD), set with the profile when the game data loads.

	tanks::ObjectsFactory::GetInstance();
	tanks::CommandsManager::GetInstance();
	tanks::KeyboardController::GetInstance();

	std::string ShowcaseFolder;
	if ( tanks::tests::IsShowcaseMode(ShowcaseFolder) )
	{
		tanks::tests::RunShowcase(ShowcaseFolder);
		return true;
	}
	if ( tanks::tests::IsTestMode() )
	{
		tanks::tests::RunAutomaticTests();
		return true;
	}

	DirectorInstance->runWithScene(tanks::view::cocos::LoadingScene::CreateScene());
	return true;
}

void AppDelegate::applicationDidEnterBackground()
{
	cocos2d::Director::getInstance()->stopAnimation();
}

void AppDelegate::applicationWillEnterForeground()
{
	cocos2d::Director::getInstance()->startAnimation();
}
