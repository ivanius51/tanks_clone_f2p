#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "Structs/PixelPoint.h"
#include "templates/TFunction.h"

namespace game
{
enum class EWindowMode
{
	Windowed = 0,
	Borderless = 1,
	Count
};

struct PixelSize
{
	int Width;
	int Height;
};

struct PixelRect
{
	int Left;
	int Top;
	int Right;
	int Bottom;
};

/** The texture set in use; a bigger set has more pixels per tile (32, 64, 128 for the same sprite). */
enum class ETextureQuality
{
	Sd,
	Hd,
	FullHd,
};
constexpr int TextureQualityCount = 3;

/** Where the texture sets live and how much bigger each one is (configs/platform/display.json). */
struct TextureQualityConfig
{
	std::string SdFolder;
	std::string HdFolder;
	std::string FullHdFolder;
	float HdScale = 1.0f;
	float FullHdScale = 1.0f;

	const std::string& GetFolder(ETextureQuality InQuality) const;
	float GetScale(ETextureQuality InQuality) const;
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
void ApplyWindowMode(EWindowMode InMode, const std::optional<PixelPoint>& InPosition = std::nullopt);
constexpr const char* GameWindowShownEvent = "tanks_game_window_shown";
void SetNextGameWindowStartsHidden(bool bInStartsHidden);
void StopCloakingNewWindows();
void KeepGameWindowCloaked();
void HideGameWindowUntilPlaced(bool bInActivateWhenShown, float InMinHiddenSeconds = 0.0f);
bool IsGameWindowHiddenUntilPlaced();
PixelRect ComputeSplashRect(PixelSize InSize, const PixelRect& InWorkArea);
PixelSize GetSplashSize(int InWidth);
PixelPoint KeepWindowInArea(PixelPoint InPosition, PixelSize InWindow, const PixelRect& InArea);
PixelPoint GetWindowPosition();
void MoveWindowTo(PixelPoint InPosition);
void WatchWindowMoves(const TFunction<void(PixelPoint), UiCallbackInlineSize>& InOnMoved);

constexpr const char* ContentScaleChangingEvent = "tanks_content_scale_changing";
constexpr const char* ContentScaleChangedEvent = "tanks_content_scale_changed";
const TextureQualityConfig& GetTextureQualityConfig();
std::string MapAtlasPath(std::string_view InAtlas, bool bInHd, const TextureQualityConfig& InConfig);
std::string MapAtlasPath(std::string_view InAtlas, ETextureQuality InQuality, const TextureQualityConfig& InConfig);
void ApplyTextureQuality(ETextureQuality InQuality);
ETextureQuality GetTextureQuality();
void ApplyHdTextures(bool bInHd);
bool IsHdTextures();
void MoveWindowToPrimaryScreen();
} // namespace game
