#pragma once

#include <string>
#include <vector>

#include "model/ConfigNode.h"
#include "model/MathTypes.h"
#include "model/ScreenLayout.h"

namespace tanks::viewmodel
{

/** Colors of the HUD (configs/ui/hud.json, "colors"). */
struct HudColors
{
	model::Color PanelFill{ 18, 20, 28 };
	model::Color PanelBorder{ 89, 102, 128 };
	model::Color BarBackground{ 41, 43, 54 };
	model::Color HealthColor{ 77, 204, 77 };
	model::Color HealthLowColor{ 230, 77, 64 };
	model::Color AmmoColor{ 242, 179, 51 };
	model::Color AmmoEmptyColor{ 204, 64, 51 };
	model::Color ReloadColor{ 230, 230, 230 };
	model::Color EnemiesColor{ 217, 89, 77 };
	model::Color SlotFill{ 26, 28, 38 };
	model::Color SlotBorder{ 128, 140, 166 };
	model::Color SlotReadyBorder{ 242, 204, 77 };
	model::Color CooldownShade{ 0, 0, 0, 173 };
};

/**
 * Layout of the level HUD (configs/ui/hud.json): the top strip and the player panel, each stuck to a screen anchor
 * so they reach the window edges at any aspect ratio, plus the ability slots and the banner.
 * The initial values are the fallbacks for missing keys.
 */
struct HudLayout
{
	HudColors Colors;

	model::ScreenAnchor TopAnchor = model::ScreenAnchor::TopLeft;
	float TopPadding = 16.0f;
	float TopFontSize = 20.0f;
	float TopSmallFontSize = 18.0f;
	float WaveX = 300.0f;
	float EnemiesX = 470.0f;
	float EnemiesBarX = 640.0f;
	float EnemiesBarWidth = 130.0f;
	float EnemiesBarHeight = 12.0f;
	float BandBorder = 2.0f;

	model::ScreenAnchor PanelAnchor = model::ScreenAnchor::BottomLeft;
	float PortraitX = 16.0f;
	float PortraitSize = 88.0f;
	float PortraitInset = 20.0f;
	float FrameBorder = 2.0f;
	float BarsX = 124.0f;
	float BarsWidth = 300.0f;
	float HealthY = 74.0f;
	float HealthHeight = 24.0f;
	float AmmoY = 46.0f;
	float AmmoHeight = 20.0f;
	float ReloadY = 36.0f;
	float ReloadHeight = 5.0f;
	float StatsY = 17.0f;
	float HealthFontSize = 17.0f;
	float AmmoFontSize = 15.0f;
	float StatsFontSize = 17.0f;
	int HealthLowPercent = 30;

	float SlotsX = 506.0f;
	float SlotStep = 88.0f;
	float SlotSize = 72.0f;
	float SlotIconSize = 44.0f;
	float SlotIconOffsetY = 4.0f;
	int MaxSlots = 3;
	float KeyFontSize = 17.0f;
	float LongKeyFontSize = 13.0f;
	float SwitchButtonWidth = 30.0f;
	float SwitchButtonHeight = 40.0f;
	float SwitchKeyFontSize = 12.0f;
	float SwitchIconInset = 6.0f;
	float SwitchIconThickness = 2.0f;
	float KeyInsetX = 4.0f;
	float KeyInsetY = 2.0f;
	float TimerFontSize = 26.0f;
	float TimerMinSeconds = 0.05f;

	float BannerFontSize = 48.0f;
	float BannerFadeIn = 0.3f;
	float BannerHold = 1.4f;
	float BannerFadeOut = 0.5f;

	// OutMissing gets the path of every missing key (its default is kept).
	static HudLayout FromConfig(const model::IConfigNode* InNode, std::vector<std::string>& OutMissing);
};

} // namespace tanks::viewmodel
