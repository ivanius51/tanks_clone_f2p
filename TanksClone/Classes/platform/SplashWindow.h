#pragma once

#include "services/DisplaySettings.h"

struct HWND__;

namespace tanks
{

/**
 * The startup splash: a small borderless native window with the game's name, centred on the primary screen.
 * It shows while the game window stays hidden and the settings load (see HideGameWindowUntilPlaced).
 * Painted with plain GDI, so it needs neither the engine nor OpenGL. Windows only: elsewhere Open() does nothing.
 */
class SplashWindow
{
public:
	SplashWindow() = default;
	~SplashWindow();
	SplashWindow(const SplashWindow&) = delete;
	SplashWindow& operator=(const SplashWindow&) = delete;

	// False if the window could not be made.
	bool Open(PixelSize InSize, bool bInActivate);
	void Close();
	bool IsOpen() const;
	// The window's outer rectangle on the desktop; empty when closed.
	PixelRect GetRect() const;

private:
	HWND__* Handle = nullptr;
};

} // namespace tanks
