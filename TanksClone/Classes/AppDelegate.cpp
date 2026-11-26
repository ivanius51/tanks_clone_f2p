#include "AppDelegate.h"

#include <array>
#include <string>

#include "core/BuildInfo.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "game/CommandsManager.h"
#include "game/ObjectsFactory.h"
#include "scenes/LoadingScene.h"
#include "services/AudioManager.h"
#include "services/DisplaySettings.h"
#include "services/InputManager.h"
#include "services/UiStyle.h"
#include "tests/CrashTests.h"
#include "tests/Showcase.h"
#include "tests/TestRunner.h"
#include "ui/ScreenFit.h"

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
#include <mmsystem.h>
#endif

namespace
{
	const cocos2d::Size DesignResolutionSize(game::DesignWidth, game::DesignHeight);

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
		game::GameContext::Get<game::InputManager>().AddListener([](const game::model::InputEvent& InEvent)
		{
			if ( InEvent.bPressed && InEvent.Action == game::model::InputAction::ToggleStats )
			{
				auto DirectorInstance = cocos2d::Director::getInstance();
				DirectorInstance->setDisplayStats(!DirectorInstance->isDisplayStats());
			}
		});
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
	game::GameContext::Get().Initialize();
	// Sent before the Director clears its scheduler and listeners: managers still unhook from a live engine.
	DirectorInstance->getEventDispatcher()->addCustomEventListener(cocos2d::Director::EVENT_RESET, [](cocos2d::EventCustom*)
	{
		game::GameContext::Get().Shutdown();
	});
	std::string ShowcaseFolder;
	const bool bShowcase = game::tests::IsShowcaseMode(ShowcaseFolder);
	// A normal start opens a small splash; the automatic runs need the full window from the first frame.
	const bool bSplash = !bShowcase && !game::tests::IsTestMode() && !game::tests::IsCrashProbe();
	auto Glview = DirectorInstance->getOpenGLView();
	if ( !Glview )
	{
		// A background test run keeps its window cloaked for good: it runs and draws, but never shows.
		game::SetNextGameWindowStartsHidden(bSplash || game::tests::IsBackgroundMode());
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32) || (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) || (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
		Glview = cocos2d::GLViewImpl::createWithRect(WindowTitle, cocos2d::Rect(0, 0, DesignResolutionSize.width, DesignResolutionSize.height));
#else
		glview = GLViewImpl::create(WINDOW_TITLE);
#endif
		DirectorInstance->setOpenGLView(Glview);
	}
	if ( bSplash )
	{
		const game::viewmodel::SplashStyle& SplashLook = game::GameContext::Get<game::UiStyle>().GetSplash();
		game::HideGameWindowUntilPlaced(true, SplashLook.MinShowTime);
		Splash.Open(game::GetSplashSize(SplashLook.Width), true, SplashLook);
		DirectorInstance->getEventDispatcher()->addCustomEventListener(game::GameWindowShownEvent, [this](cocos2d::EventCustom*) { Splash.Close(); });
	}
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	if ( game::tests::IsBackgroundMode() )
	{
		game::KeepGameWindowCloaked();
		SendWindowToBackground(Glview);
	}
#endif

	// The engine's stats (FPS, draw calls) cover the player's panel: hidden, F3 shows them in Debug and Test builds.
	DirectorInstance->setDisplayStats(false);
	if ( game::bHasDebugTools )
	{
		AddDebugStatsToggle();
	}

	game::ApplyMenuFrameRate();
	game::view::cocos::ApplyScreenFit();
	game::view::cocos::WatchWindowResizes();
	// The content scale follows the texture set (HD or SD), set with the profile when the game data loads.

	if ( game::tests::StartCrashProbe() )
	{
		DirectorInstance->runWithScene(cocos2d::Scene::create());
		return true;
	}
	if ( bShowcase )
	{
		game::tests::RunShowcase(ShowcaseFolder);
		return true;
	}
	if ( game::tests::IsTestMode() )
	{
		game::tests::RunAutomaticTests();
		return true;
	}

	DirectorInstance->runWithScene(game::view::cocos::LoadingScene::CreateScene());
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
