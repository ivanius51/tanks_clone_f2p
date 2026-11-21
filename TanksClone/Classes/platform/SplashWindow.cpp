#include "platform/SplashWindow.h"

#include "cocos2d.h"

namespace tanks
{

#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
namespace
{
	constexpr const wchar_t* ClassName = L"TanksCloneSplash";
	constexpr const wchar_t* Title = L"TANKS CLONE";
	constexpr const wchar_t* Status = L"Loading...";
	constexpr const wchar_t* FontFace = L"Segoe UI";
	const COLORREF Background = RGB(14, 17, 24);
	const COLORREF Accent = RGB(255, 205, 70);
	const COLORREF Dim = RGB(150, 150, 160);
	const COLORREF Border = RGB(70, 80, 100);
	// Font heights as parts of the window height.
	constexpr int TitleHeightDivisor = 7;
	constexpr int StatusHeightDivisor = 18;

	void PaintText(HDC InContext, const wchar_t* InText, const RECT& InArea, const int InHeight, const int InWeight, const COLORREF InColor)
	{
		// A negative height asks for the character height in pixels, not the cell height.
		HFONT Font = CreateFontW(-InHeight, 0, 0, 0, InWeight, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
			CLEARTYPE_QUALITY, DEFAULT_PITCH, FontFace);
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

		HBRUSH Frame = CreateSolidBrush(Border);
		FillRect(Context, &Client, Frame);
		DeleteObject(Frame);
		RECT Inner = { Client.left + 2, Client.top + 2, Client.right - 2, Client.bottom - 2 };
		HBRUSH Fill = CreateSolidBrush(Background);
		FillRect(Context, &Inner, Fill);
		DeleteObject(Fill);

		SetBkMode(Context, TRANSPARENT);
		const int Height = Client.bottom - Client.top;
		const RECT TitleArea = { Client.left, Client.top, Client.right, Client.top + Height * 3 / 5 };
		const RECT StatusArea = { Client.left, Client.top + Height * 3 / 5, Client.right, Client.top + Height * 4 / 5 };
		PaintText(Context, Title, TitleArea, Height / TitleHeightDivisor, FW_BOLD, Accent);
		PaintText(Context, Status, StatusArea, Height / StatusHeightDivisor, FW_NORMAL, Dim);
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

bool SplashWindow::Open(const PixelSize InSize, const bool bInActivate)
{
#if (CC_TARGET_PLATFORM == CC_PLATFORM_WIN32)
	if ( Handle || !RegisterSplashClass() )
	{
		return Handle != nullptr;
	}
	MONITORINFO Info = {};
	Info.cbSize = sizeof(Info);
	// The primary monitor is the one that holds the desktop's origin.
	GetMonitorInfo(MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY), &Info);
	const PixelRect Rect = ComputeSplashRect(InSize, { Info.rcWork.left, Info.rcWork.top, Info.rcWork.right, Info.rcWork.bottom });

	// WS_EX_TOOLWINDOW keeps the splash off the taskbar: the game window takes that place when it appears.
	Handle = CreateWindowExW(WS_EX_TOOLWINDOW, ClassName, Title, WS_POPUP, Rect.Left, Rect.Top, Rect.Right - Rect.Left, Rect.Bottom - Rect.Top,
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

} // namespace tanks
