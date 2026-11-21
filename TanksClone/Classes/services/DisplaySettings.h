#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>


namespace tanks
{

// How the game window is shown.
enum class WindowMode
{
	Windowed = 0,
	Borderless = 1 // the whole screen at the desktop's resolution, without a frame (no video mode change)
};

struct PixelSize
{
	int Width;
	int Height;
};

struct PixelPoint
{
	int X;
	int Y;
};

struct PixelRect
{
	int Left;
	int Top;
	int Right;
	int Bottom;
};

// Where the two texture sets live and how much bigger the HD one is (configs/display.json).
struct TextureQualityConfig
{
	std::string SdFolder;
	std::string HdFolder;
	float HdScale = 1.0f;
};

// Frame rate limits the player can choose come from configs/display.json; 0 means unlimited.
// The option after InFps (wraps around). Unknown values give the first option.
int NextFpsOption(int InFps);

// The player's limit applies while a level is played; menus always run at MenuFps.
void ApplyFpsLimit(int InFps);
constexpr int MenuFps = 60;
void ApplyMenuFrameRate();

// Vertical sync (the frame rate follows the screen) is a setting of its own; it applies at once.
void SetVerticalSyncEnabled(bool bInEnabled);
bool IsVerticalSyncEnabled();
// The vertical sync interval the graphics driver reports (0 = off, 1 = on), -1 if it cannot tell.
int GetVerticalSyncInterval();

// Window sizes for the windowed mode, in percent of the design size; 0 = auto (fits the screen).
int NextWindowScale(int InScalePercent);
// The window's inner size for a scale: the design size scaled, never bigger than InArea (aspect kept).
PixelSize ComputeWindowSize(int InScalePercent, PixelSize InDesign, PixelSize InArea);
// What applyWindowMode gives in the windowed mode on the current screen.
PixelSize GetExpectedWindowedSize(int InScalePercent);
PixelSize GetScreenSize();
// The inner size of the game window as the operating system sees it.
PixelSize GetWindowClientSize();
// Switches the window live; the screens fill any aspect ratio (see view::cocos::ApplyScreenFit).
// A windowed window goes to InPosition (its outer top-left corner) if given, else to the centre of its screen.
void ApplyWindowMode(WindowMode InMode, int InScalePercent, const std::optional<PixelPoint>& InPosition = std::nullopt);
// The startup splash: a small borderless window (platform::SplashWindow) shows while the profile is read; the
// game window stays hidden until ApplyWindowMode has given it the saved mode, size and place, then shows it and
// sends GameWindowShownEvent. The player never sees the game window jump.
constexpr const char* GameWindowShownEvent = "tanks_game_window_shown";
void HideGameWindowUntilPlaced(bool bInActivateWhenShown);
bool IsGameWindowHiddenUntilPlaced();
// Where a splash of InSize goes inside InWorkArea: centred, never bigger than the area.
PixelRect ComputeSplashRect(PixelSize InSize, const PixelRect& InWorkArea);
// The splash's inner size: a small copy of the design area.
PixelSize GetSplashSize();
// The corner that keeps a window of InWindow size whole inside InArea (moved in, never resized; a window
// bigger than the area keeps its top-left corner inside it).
PixelPoint KeepWindowInArea(PixelPoint InPosition, PixelSize InWindow, const PixelRect& InArea);
// The window's outer top-left corner on the desktop.
PixelPoint GetWindowPosition();
// Moves the window (outer top-left corner), kept whole on the screen nearest to InPosition.
void MoveWindowTo(PixelPoint InPosition);
// Calls InOnMoved with the window's new corner each time the player stops dragging it (Windows only).
void WatchWindowMoves(const std::function<void(PixelPoint)>& InOnMoved);

// HD or SD textures. The atlases are named by their SD path ("textures/..."); HD maps them to the HD folder
// and raises the content scale, so sprites keep their size on screen. Applies at once: loaded atlases reload.
// Sent before the font atlas cache is emptied; the user data is a view::cocos::FontAtlasHold for the atlases in use.
constexpr const char* ContentScaleChangingEvent = "tanks_content_scale_changing";
// Sent after a texture quality switch has changed the content scale and emptied the font atlas cache.
constexpr const char* ContentScaleChangedEvent = "tanks_content_scale_changed";
const TextureQualityConfig& GetTextureQualityConfig();
std::string MapAtlasPath(std::string_view InAtlas, bool bInHd, const TextureQualityConfig& InConfig);
void ApplyHdTextures(bool bInHd);
bool IsHdTextures();
// For the automatic runs: a remote or virtual second screen can hold the frame rate down.
void MoveWindowToPrimaryScreen();

} // namespace tanks
