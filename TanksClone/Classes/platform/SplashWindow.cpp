#include "platform/SplashWindow.h"

#include <algorithm>
#include <string>

#include "cocos2d.h"

namespace game
{

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
namespace
{
	constexpr const wchar_t* ClassName = L"TanksCloneSplash";

	// What the window procedure paints: set by Open, read by WM_PAINT.
	viewmodel::SplashStyle GStyle;
	std::wstring GTitle;
	std::wstring GStatus;
	std::wstring GFontFace;

	std::wstring Widen(const std::string& InUtf8)
	{
		const int Length = MultiByteToWideChar(CP_UTF8, 0, InUtf8.c_str(), static_cast<int>(InUtf8.size()), nullptr, 0);
		std::wstring Result(static_cast<std::size_t>(std::max(0, Length)), L'\0');
		if ( Length > 0 )
		{
			MultiByteToWideChar(CP_UTF8, 0, InUtf8.c_str(), static_cast<int>(InUtf8.size()), Result.data(), Length);
		}
		return Result;
	}

	COLORREF ToColorRef(const model::Color& InColor)
	{
		return RGB(InColor.R, InColor.G, InColor.B);
	}

	void PaintText(HDC InContext, const wchar_t* InText, const RECT& InArea, const int InHeight, const int InWeight, const COLORREF InColor)
	{
		// A negative height asks for the character height in pixels, not the cell height.
		HFONT Font = CreateFontW(-InHeight, 0, 0, 0, InWeight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY, DEFAULT_PITCH, GFontFace.c_str());
		HGDIOBJ Previous = SelectObject(InContext, Font);
		SetTextColor(InContext, InColor);
		RECT Area = InArea;
		::DrawTextW(InContext, InText, -1, &Area, DT_CENTER | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);
		SelectObject(InContext, Previous);
		DeleteObject(Font);
	}

	void Paint(HWND InWindow)
	{
		PAINTSTRUCT Paint = {};
		HDC Context = BeginPaint(InWindow, &Paint);
		RECT Client = {};
		GetClientRect(InWindow, &Client);

		HBRUSH Frame = CreateSolidBrush(ToColorRef(GStyle.Border));
		FillRect(Context, &Client, Frame);
		DeleteObject(Frame);
		const int Edge = GStyle.BorderWidth;
		RECT Inner = { Client.left + Edge, Client.top + Edge, Client.right - Edge, Client.bottom - Edge };
		HBRUSH Fill = CreateSolidBrush(ToColorRef(GStyle.Background));
		FillRect(Context, &Inner, Fill);
		DeleteObject(Fill);

		SetBkMode(Context, TRANSPARENT);
		const int Height = Client.bottom - Client.top;
		const int TitleEnd = Client.top + static_cast<int>(Height * GStyle.TitleAreaEnd);
		const int StatusEnd = Client.top + static_cast<int>(Height * GStyle.StatusAreaEnd);
		const RECT TitleArea = { Client.left, Client.top, Client.right, TitleEnd };
		const RECT StatusArea = { Client.left, TitleEnd, Client.right, StatusEnd };
		PaintText(Context, GTitle.c_str(), TitleArea, Height / std::max(1, GStyle.TitleHeightDivisor), FW_BOLD, ToColorRef(GStyle.TitleColor));
		PaintText(Context, GStatus.c_str(), StatusArea, Height / std::max(1, GStyle.StatusHeightDivisor), FW_NORMAL, ToColorRef(GStyle.StatusColor));
		EndPaint(InWindow, &Paint);
	}

	LRESULT CALLBACK SplashProc(HWND InWindow, UINT InMessage, WPARAM InWParam, LPARAM InLParam)
	{
		switch ( InMessage )
		{
			case WM_PAINT:
				Paint(InWindow);
				return 0;
			case WM_ERASEBKGND:
				return 1; // WM_PAINT fills everything; no flicker from a default erase
			default:
				return DefWindowProcW(InWindow, InMessage, InWParam, InLParam);
		}
	}

	bool RegisterSplashClass()
	{
		WNDCLASSEXW Class = {};
		Class.cbSize = sizeof(Class);
		Class.lpfnWndProc = SplashProc;
		Class.hInstance = GetModuleHandleW(nullptr);
		Class.hCursor = LoadCursorW(nullptr, IDC_ARROW);
		Class.lpszClassName = ClassName;
		// Registering twice fails with ERROR_CLASS_ALREADY_EXISTS, which is fine.
		return RegisterClassExW(&Class) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
	}
}
#endif

SplashWindow::~SplashWindow()
{
	Close();
}

bool SplashWindow::Open(const PixelSize InSize, const bool bInActivate, const viewmodel::SplashStyle& InStyle)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	if ( Handle || !RegisterSplashClass() )
	{
		return Handle != nullptr;
	}
	GStyle = InStyle;
	GTitle = Widen(InStyle.Title);
	GStatus = Widen(InStyle.Status);
	GFontFace = Widen(InStyle.FontFace);
	MONITORINFO Info = {};
	Info.cbSize = sizeof(Info);
	// The primary monitor is the one that holds the desktop's origin.
	GetMonitorInfo(MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY), &Info);
	const PixelRect Rect = ComputeSplashRect(InSize, { Info.rcWork.left, Info.rcWork.top, Info.rcWork.right, Info.rcWork.bottom });

	// WS_EX_TOOLWINDOW keeps the splash off the taskbar: the game window takes that place when it appears.
	Handle = CreateWindowExW(WS_EX_TOOLWINDOW, ClassName, GTitle.c_str(), WS_POPUP, Rect.Left, Rect.Top, Rect.Right - Rect.Left, Rect.Bottom - Rect.Top,
		nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
	if ( !Handle )
	{
		return false;
	}
	ShowWindow(Handle, bInActivate ? SW_SHOW : SW_SHOWNOACTIVATE);
	UpdateWindow(Handle);
	return true;
#else
	(void)InSize;
	(void)bInActivate;
	(void)InStyle;
	return false;
#endif
}

void SplashWindow::Close()
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	if ( Handle )
	{
		DestroyWindow(Handle);
		Handle = nullptr;
	}
#endif
}

bool SplashWindow::IsOpen() const
{
	return Handle != nullptr;
}

PixelRect SplashWindow::GetRect() const
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	RECT Outer = {};
	if ( Handle && GetWindowRect(Handle, &Outer) )
	{
		return { Outer.left, Outer.top, Outer.right, Outer.bottom };
	}
#endif
	return { 0, 0, 0, 0 };
}

} // namespace game
