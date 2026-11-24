#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace tanks
{
enum class WindowMode
{
	Windowed = 0,
	Borderless = 1
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

/** Where the two texture sets live and how much bigger the HD one is (configs/platform/display.json). */
struct TextureQualityConfig
{
	std::string SdFolder;
	std::string HdFolder;
	float HdScale = 1.0f;
};

int NextFpsOption(int InFps);

void ApplyFpsLimit(int InFps);
constexpr int MenuFps = 60;
void ApplyMenuFrameRate();

void SetVerticalSyncEnabled(bool bInEnabled);
bool IsVerticalSyncEnabled();
int GetVerticalSyncInterval();

int NextMapScale(int InScalePercent);
PixelSize ComputeWindowSize(PixelSize InDesign, PixelSize InArea);
PixelSize GetExpectedWindowedSize();
PixelSize GetScreenSize();
PixelSize GetWindowClientSize();
void ApplyWindowMode(WindowMode InMode, const std::optional<PixelPoint>& InPosition = std::nullopt);
constexpr const char* GameWindowShownEvent = "tanks_game_window_shown";
void HideGameWindowUntilPlaced(bool bInActivateWhenShown, float InMinHiddenSeconds = 0.0f);
bool IsGameWindowHiddenUntilPlaced();
PixelRect ComputeSplashRect(PixelSize InSize, const PixelRect& InWorkArea);
PixelSize GetSplashSize(int InWidth);
PixelPoint KeepWindowInArea(PixelPoint InPosition, PixelSize InWindow, const PixelRect& InArea);
PixelPoint GetWindowPosition();
void MoveWindowTo(PixelPoint InPosition);
void WatchWindowMoves(const std::function<void(PixelPoint)>& InOnMoved);

constexpr const char* ContentScaleChangingEvent = "tanks_content_scale_changing";
constexpr const char* ContentScaleChangedEvent = "tanks_content_scale_changed";
const TextureQualityConfig& GetTextureQualityConfig();
std::string MapAtlasPath(std::string_view InAtlas, bool bInHd, const TextureQualityConfig& InConfig);
void ApplyHdTextures(bool bInHd);
bool IsHdTextures();
void MoveWindowToPrimaryScreen();
} // namespace tanks
