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
	BorderlessWindow = 2,
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

/** The texture set in use; HD has twice the pixels per tile of SD (32 and 64 for the same sprite). */
enum class ETextureQuality
{
	Sd,
	Hd,
};
constexpr int TextureQualityCount = 2;

/** Where the texture sets live, how much bigger each one is, and the fixed engine content scale (configs/platform/display.json). */
struct TextureQualityConfig
{
	std::string SdFolder;
	std::string HdFolder;
	float HdScale = 1.0f;
	/** Pixels per point everything is rasterized at (fonts, UI, sprites); a smaller set is enlarged to it when it loads. */
	float ContentScale = 1.0f;

	const std::string& GetFolder(ETextureQuality InQuality) const;
	float GetScale(ETextureQuality InQuality) const;
	/** The whole factor a set's pixels are enlarged by (nearest) to reach the content scale; 1 when it already does. */
	int GetUpscale(ETextureQuality InQuality) const;
};

/** The look of the game, chosen in the settings: pixel art, classic or a 3D look. */
enum class EArtStyle
{
	PixelArt,
	Classic,
	Look3D,
};
constexpr int ArtStyleCount = 3;

/** What an art style switches on ("art_styles" in configs/platform/display.json). */
struct ArtStyleDef
{
	ETextureQuality Textures = ETextureQuality::Hd;
	bool bSmooth = true;
	bool bWallMerge = true;
	bool bTilt = false;
	bool bShadows = false;
	bool bWallSides = false;
};

/** The definition of a profile art style; an out of range value is clamped. */
const ArtStyleDef& GetArtStyleDef(int InStyle);

/** Applies a profile texture setting; values of removed sets fall back to the biggest one. */
void ApplyTextureQualitySetting(int InSetting);

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
/** The frameless window of the wanted size: never bigger than the screen, centred on it (it may cover the taskbar). */
PixelRect ComputeBorderlessRect(PixelSize InWanted, const PixelRect& InScreen);
/** Makes the game window frameless at this size and centres it on its screen; the next ApplyWindowMode with another mode brings the frame back. */
void ResizeBorderlessWindow(PixelSize InSize);
bool IsGameWindowFrameless();
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
