#include "services/DisplaySettings.h"

#include <algorithm>
#include <chrono>
#include <vector>

#include "2d/CCFontAtlas.h"
#include "2d/CCFontAtlasCache.h"
#include "cocos2d.h"

#include "core/ConfigReader.h"
#include "core/GameConstants.h"
#include "core/GameContext.h"
#include "core/ObjectCast.h"
#include "core/ResourceIds.h"
#include "services/ConfigRefs.h"
#include "services/ResourceManager.h"
#include "ui/FontAtlasHold.h"
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#endif

#if (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) || (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
#include "glfw3.h"
#endif

namespace tanks
{

namespace
{
	const std::string& DisplayConfig()
	{
		static const std::string Path = config_refs::ResolveId(ids::configs::Display);
		return Path;
	}
	// "Unlimited" still needs a (tiny) interval for the engine.
	constexpr float UnlimitedInterval = 1.0f / 2000.0f;
	// "Auto" fills at most this part of the free desktop area.
	constexpr float AutoScreenShare = 0.9f;

	bool GVerticalSync = true;
	bool GHiddenUntilPlaced = false;
	bool GActivateWhenShown = false;
	std::chrono::steady_clock::time_point GShowNotBefore;
	bool GShowScheduled = false;
	bool GHdTextures = false;
	// Any number: names the window subclass that watches the window moves.
	constexpr UINT_PTR MoveWatcherId = 0x7A4C;

	TFunction<void(PixelPoint)>& MoveCallback()
	{
		static TFunction<void(PixelPoint)> Callback;
		return Callback;
	}

	std::string ReadString(const cocos2d::ValueMap& InConfig, const std::string& InKey, const std::string& InDefault)
	{
		const auto Found = InConfig.find(InKey);
		if ( Found == InConfig.end() || Found->second.getType() != cocos2d::Value::Type::STRING )
		{
			CCLOGERROR("DisplaySettings: \"%s\" is missing in %s", InKey.c_str(), DisplayConfig().c_str());
			return InDefault;
		}
		return Found->second.asString();
	}

	std::vector<int> ReadOptions(const std::string& InKey)
	{
		std::vector<int> Options;
		const cocos2d::ValueMap& Config = GameContext::Get().GetManager<ConfigReader>().GetObjectData(DisplayConfig()).Values;
		const auto Found = Config.find(InKey);
		if ( Found != Config.end() && Found->second.getType() == cocos2d::Value::Type::VECTOR )
		{
			for ( const cocos2d::Value& Option : Found->second.asValueVector() )
			{
				Options.push_back(Option.asInt());
			}
		}
		if ( Options.empty() )
		{
			CCLOGERROR("DisplaySettings: \"%s\" is missing in %s", InKey.c_str(), DisplayConfig().c_str());
			Options.push_back(0);
		}
		return Options;
	}

	const std::vector<int>& FpsOptions()
	{
		static const std::vector<int> Options = ReadOptions("fps_options");
		return Options;
	}

	const std::vector<int>& MapScaleOptions()
	{
		static const std::vector<int> Options = ReadOptions("map_scale_options");
		return Options;
	}

	int NextOption(const std::vector<int>& InOptions, const int InValue)
	{
		const auto Found = std::find(InOptions.begin(), InOptions.end(), InValue);
		if ( Found == InOptions.end() )
		{
			return InOptions.front();
		}
		const auto Next = std::next(Found);
		return Next != InOptions.end() ? *Next : InOptions.front();
	}

	void SetVerticalSync(const bool bInEnabled)
	{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		// The engine's GLFW lives inside libcocos2d.dll; a GLFW call from the game would reach another,
		// uninitialised copy. The WGL extension works on the OpenGL context current on this thread.
		typedef BOOL (WINAPI *SwapIntervalFunction)(int);
		auto SwapInterval = reinterpret_cast<SwapIntervalFunction>(wglGetProcAddress("wglSwapIntervalEXT"));
		if ( SwapInterval )
		{
			SwapInterval(bInEnabled ? 1 : 0);
		}
#elif (CC_TARGET_PLATFORM == CC_PLATFORM_MAC) || (CC_TARGET_PLATFORM == CC_PLATFORM_LINUX)
		if ( glfwGetCurrentContext() )
		{
			glfwSwapInterval(bInEnabled ? 1 : 0);
		}
#else
		(void)bInEnabled;
#endif
	}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	HWND GameWindow()
	{
		auto View = CheckedCast<cocos2d::GLViewImpl>(cocos2d::Director::getInstance()->getOpenGLView());
		return View ? View->getWin32Window() : nullptr;
	}

	MONITORINFO MonitorOf(HWND InWindow)
	{
		MONITORINFO Info = {};
		Info.cbSize = sizeof(Info);
		GetMonitorInfo(MonitorFromWindow(InWindow, MONITOR_DEFAULTTONEAREST), &Info);
		return Info;
	}

	PixelRect ToPixelRect(const RECT& InRect)
	{
		return { InRect.left, InRect.top, InRect.right, InRect.bottom };
	}

	// The engine's GLFW owns the window procedure; a subclass sees the messages first and passes them on.
	LRESULT CALLBACK WatchMoves(HWND InWindow, UINT InMessage, WPARAM InWParam, LPARAM InLParam, UINT_PTR, DWORD_PTR)
	{
		if ( InMessage == WM_EXITSIZEMOVE && MoveCallback() )
		{
			MoveCallback()(GetWindowPosition());
		}
		return DefSubclassProc(InWindow, InMessage, InWParam, InLParam);
	}
#endif
}

int NextFpsOption(const int InFps)
{
	return NextOption(FpsOptions(), InFps);
}

int NextMapScale(const int InScalePercent)
{
	return NextOption(MapScaleOptions(), InScalePercent);
}

PixelSize ComputeWindowSize(const PixelSize InDesign, const PixelSize InArea)
{
	const float Fit = std::min(static_cast<float>(InArea.Width) / InDesign.Width, static_cast<float>(InArea.Height) / InDesign.Height);
	const float Scale = Fit * AutoScreenShare;
	return { static_cast<int>(InDesign.Width * Scale + 0.5f), static_cast<int>(InDesign.Height * Scale + 0.5f) };
}

void SetVerticalSyncEnabled(const bool bInEnabled)
{
	GVerticalSync = bInEnabled;
	SetVerticalSync(bInEnabled);
}

bool IsVerticalSyncEnabled()
{
	return GVerticalSync;
}

void ApplyFpsLimit(const int InFps)
{
	cocos2d::Director::getInstance()->setAnimationInterval(InFps > 0 ? 1.0f / InFps : UnlimitedInterval);
	SetVerticalSync(GVerticalSync);
}

int GetVerticalSyncInterval()
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	// Answers for the OpenGL context current on this thread; the extension may be missing.
	typedef int (WINAPI *GetSwapIntervalFunction)();
	auto GetSwapIntervalExt = reinterpret_cast<GetSwapIntervalFunction>(wglGetProcAddress("wglGetSwapIntervalEXT"));
	return GetSwapIntervalExt ? GetSwapIntervalExt() : -1;
#else
	return -1;
#endif
}

void ApplyMenuFrameRate()
{
	ApplyFpsLimit(MenuFps);
}

PixelSize GetScreenSize()
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	const MONITORINFO Info = MonitorOf(GameWindow());
	return { Info.rcMonitor.right - Info.rcMonitor.left, Info.rcMonitor.bottom - Info.rcMonitor.top };
#else
	const cocos2d::Size CurrentSize = cocos2d::Director::getInstance()->getOpenGLView()->getFrameSize();
	return { static_cast<int>(CurrentSize.width), static_cast<int>(CurrentSize.height) };
#endif
}

PixelSize GetWindowClientSize()
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	RECT ClientRect = {};
	GetClientRect(GameWindow(), &ClientRect);
	return { ClientRect.right - ClientRect.left, ClientRect.bottom - ClientRect.top };
#else
	const cocos2d::Size CurrentSize = cocos2d::Director::getInstance()->getOpenGLView()->getFrameSize();
	return { static_cast<int>(CurrentSize.width), static_cast<int>(CurrentSize.height) };
#endif
}

PixelSize GetExpectedWindowedSize()
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	const MONITORINFO Info = MonitorOf(GameWindow());
	const PixelSize Area = { Info.rcWork.right - Info.rcWork.left, Info.rcWork.bottom - Info.rcWork.top };
#else
	const PixelSize Area = GetScreenSize();
#endif
	// The design size itself: the engine's design resolution follows the window's aspect.
	return ComputeWindowSize({ static_cast<int>(DesignWidth), static_cast<int>(DesignHeight) }, Area);
}

void MoveWindowToPrimaryScreen()
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	HWND Window = GameWindow();
	if ( !Window )
	{
		return;
	}
	MONITORINFO Info = {};
	Info.cbSize = sizeof(Info);
	// The primary monitor is the one that holds the desktop's origin.
	GetMonitorInfo(MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY), &Info);
	RECT Outer = {};
	GetWindowRect(Window, &Outer);
	const RECT& Work = Info.rcWork;
	const int Left = Work.left + std::max(0, static_cast<int>((Work.right - Work.left) - (Outer.right - Outer.left)) / 2);
	const int Top = Work.top + std::max(0, static_cast<int>((Work.bottom - Work.top) - (Outer.bottom - Outer.top)) / 2);
	SetWindowPos(Window, nullptr, Left, Top, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
#endif
}

namespace
{
	void ApplyWindowModeNow(const WindowMode InMode, const std::optional<PixelPoint>& InPosition)
	{
		// Through the engine's view: it drives the GLFW copy inside libcocos2d.dll, which keeps its idea of the
		// Window (frame, size) right. Changing the Window behind its back makes the two fight over the size.
		auto View = CheckedCast<cocos2d::GLViewImpl>(cocos2d::Director::getInstance()->getOpenGLView());
		if ( !View )
		{
			return;
		}
		if ( InMode == WindowMode::Borderless )
		{
			if ( !View->isFullscreen() )
			{
				View->setFullscreen();
			}
			return;
		}

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		// The screen the Window is on now: a bigger Window may reach over to another one.
		HWND Window = GameWindow();
		const RECT Work = MonitorOf(Window).rcWork;
#endif

		const PixelSize Client = GetExpectedWindowedSize();
		View->setWindowed(Client.Width, Client.Height);

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
		if ( InPosition )
		{
			MoveWindowTo(*InPosition);
			return;
		}
		// Only moved, not resized: the Window stays whole on its screen.
		RECT Outer = {};
		GetWindowRect(Window, &Outer);
		const int Left = Work.left + std::max(0, static_cast<int>((Work.right - Work.left) - (Outer.right - Outer.left)) / 2);
		const int Top = Work.top + std::max(0, static_cast<int>((Work.bottom - Work.top) - (Outer.bottom - Outer.top)) / 2);
		SetWindowPos(Window, nullptr, Left, Top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
#else
		(void)InPosition;
#endif
	}
}

PixelRect ComputeSplashRect(const PixelSize InSize, const PixelRect& InWorkArea)
{
	const int Width = std::min(InSize.Width, InWorkArea.Right - InWorkArea.Left);
	const int Height = std::min(InSize.Height, InWorkArea.Bottom - InWorkArea.Top);
	const int Left = InWorkArea.Left + (InWorkArea.Right - InWorkArea.Left - Width) / 2;
	const int Top = InWorkArea.Top + (InWorkArea.Bottom - InWorkArea.Top - Height) / 2;
	return { Left, Top, Left + Width, Top + Height };
}

PixelSize GetSplashSize(const int InWidth)
{
	return { InWidth, static_cast<int>(InWidth * DesignHeight / DesignWidth + 0.5f) };
}

void HideGameWindowUntilPlaced(const bool bInActivateWhenShown, const float InMinHiddenSeconds)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	if ( HWND Window = GameWindow() )
	{
		ShowWindow(Window, SW_HIDE);
		GHiddenUntilPlaced = true;
		GActivateWhenShown = bInActivateWhenShown;
		GShowNotBefore = std::chrono::steady_clock::now()
			+ std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<float>(std::max(0.0f, InMinHiddenSeconds)));
	}
#else
	(void)bInActivateWhenShown;
	(void)InMinHiddenSeconds;
#endif
}

bool IsGameWindowHiddenUntilPlaced()
{
	return GHiddenUntilPlaced;
}

void ShowPlacedGameWindow()
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	HWND Window = GameWindow();
	GShowScheduled = false;
	if ( !GHiddenUntilPlaced || !Window )
	{
		return;
	}
	GHiddenUntilPlaced = false;
	ShowWindow(Window, GActivateWhenShown ? SW_SHOW : SW_SHOWNA);
	if ( GActivateWhenShown )
	{
		SetForegroundWindow(Window);
	}
	cocos2d::Director::getInstance()->getEventDispatcher()->dispatchCustomEvent(GameWindowShownEvent);
#endif
}

void ApplyWindowMode(const WindowMode InMode, const std::optional<PixelPoint>& InPosition)
{
	ApplyWindowModeNow(InMode, InPosition);
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	if ( !GHiddenUntilPlaced || GShowScheduled || !GameWindow() )
	{
		return;
	}
	const float Wait = std::chrono::duration<float>(GShowNotBefore - std::chrono::steady_clock::now()).count();
	if ( Wait <= 0.0f )
	{
		ShowPlacedGameWindow();
		return;
	}
	GShowScheduled = true;
	cocos2d::Director::getInstance()->getScheduler()->schedule([](float) { ShowPlacedGameWindow(); }, &GShowScheduled, 0.0f, 0, Wait, false,
		"tanks_show_game_window");
#endif
}

PixelPoint KeepWindowInArea(const PixelPoint InPosition, const PixelSize InWindow, const PixelRect& InArea)
{
	const int X = std::max(InArea.Left, std::min(InPosition.X, InArea.Right - InWindow.Width));
	const int Y = std::max(InArea.Top, std::min(InPosition.Y, InArea.Bottom - InWindow.Height));
	return { X, Y };
}

PixelPoint GetWindowPosition()
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	RECT Outer = {};
	GetWindowRect(GameWindow(), &Outer);
	return { Outer.left, Outer.top };
#else
	return { 0, 0 };
#endif
}

void MoveWindowTo(const PixelPoint InPosition)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	HWND Window = GameWindow();
	if ( !Window )
	{
		return;
	}
	RECT Outer = {};
	GetWindowRect(Window, &Outer);
	const PixelSize OuterSize = { Outer.right - Outer.left, Outer.bottom - Outer.top };
	// The screen the saved place is on; a screen that is gone gives the nearest one.
	const RECT Wanted = { InPosition.X, InPosition.Y, InPosition.X + OuterSize.Width, InPosition.Y + OuterSize.Height };
	MONITORINFO Info = {};
	Info.cbSize = sizeof(Info);
	GetMonitorInfo(MonitorFromRect(&Wanted, MONITOR_DEFAULTTONEAREST), &Info);
	const PixelPoint Corner = KeepWindowInArea(InPosition, OuterSize, ToPixelRect(Info.rcWork));
	SetWindowPos(Window, nullptr, Corner.X, Corner.Y, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
#else
	(void)InPosition;
#endif
}

void WatchWindowMoves(const TFunction<void(PixelPoint)>& InOnMoved)
{
	MoveCallback() = InOnMoved;
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	if ( HWND Window = GameWindow() )
	{
		SetWindowSubclass(Window, WatchMoves, MoveWatcherId, 0);
	}
#endif
}

const TextureQualityConfig& GetTextureQualityConfig()
{
	static const TextureQualityConfig Config = []()
	{
		const cocos2d::ValueMap& Values = GameContext::Get().GetManager<ConfigReader>().GetObjectData(DisplayConfig()).Values;
		TextureQualityConfig Result;
		Result.SdFolder = ReadString(Values, "sd_texture_folder", "textures/");
		Result.HdFolder = ReadString(Values, "hd_texture_folder", Result.SdFolder);
		const auto Scale = Values.find("hd_content_scale");
		Result.HdScale = Scale != Values.end() ? Scale->second.asFloat() : 1.0f;
		return Result;
	}();
	return Config;
}

std::string MapAtlasPath(const std::string_view InAtlas, const bool bInHd, const TextureQualityConfig& InConfig)
{
	const std::string_view Sd = InConfig.SdFolder;
	if ( !bInHd || InAtlas.substr(0, Sd.size()) != Sd )
	{
		return std::string(InAtlas);
	}
	return InConfig.HdFolder + std::string(InAtlas.substr(Sd.size()));
}

void ApplyHdTextures(const bool bInHd)
{
	const float Scale = bInHd ? GetTextureQualityConfig().HdScale : 1.0f;
	const bool bChanged = GHdTextures != bInHd || cocos2d::Director::getInstance()->getContentScaleFactor() != Scale;
	GHdTextures = bInHd;
	if ( !bChanged )
	{
		return;
	}
	// Frames are read in pixels and turned into points by the content scale, so it changes before the reload.
	cocos2d::Director::getInstance()->setContentScaleFactor(Scale);
	// Font atlases are rasterized at the content scale but cached by font and size only. The engine purge hands live labels
	// their old atlas back while it is still cached, so labels take a fresh one on the event, once the cache is empty.
	// The hold outlives the purge, so an atlas in use is deleted only after the labels have switched.
	view::cocos::FontAtlasHold AtlasesInUse;
	cocos2d::EventDispatcher* const Dispatcher = cocos2d::Director::getInstance()->getEventDispatcher();
	Dispatcher->dispatchCustomEvent(ContentScaleChangingEvent, &AtlasesInUse);
	cocos2d::FontAtlasCache::purgeCachedData();
	Dispatcher->dispatchCustomEvent(ContentScaleChangedEvent);
	GameContext::Get().GetManager<ResourceManager>().ReloadAtlases();
}

bool IsHdTextures()
{
	return GHdTextures;
}

} // namespace tanks
