#pragma once

#include <string>
#include <vector>

#include "model/ConfigNode.h"
#include "model/MathTypes.h"

namespace tanks::viewmodel
{

/** The main menu screen. "Top" values count down from the top of the design area. */
struct MainMenuLayout
{
	float TitleFontSize = 84.0f;
	float InfoFontSize = 22.0f;
	float TitleTop = 110.0f;
	float InfoTop = 175.0f;
	float ButtonWidth = 380.0f;
	float ButtonHeight = 52.0f;
	float ButtonSpacing = 12.0f;
	float ListOffsetY = -60.0f;
};

/** The settings list, as a screen or over a paused level. */
struct SettingsLayout
{
	float TitleFontSize = 56.0f;
	float ButtonWidth = 560.0f;
	float ButtonHeight = 48.0f;
	float ButtonSpacing = 8.0f;
	float ListCenterOffset = 40.0f;
	float TitleTopOffset = 120.0f;
	float PanelPadding = 28.0f;
	float TitleGap = 24.0f;
	int VolumeStep = 5;
};

/** The upgrades shop. Y positions count down from the top of the design area. */
struct UpgradesLayout
{
	float TitleFontSize = 56.0f;
	float NameFontSize = 28.0f;
	float InfoFontSize = 18.0f;
	float TitleTop = 80.0f;
	float CoinsTop = 150.0f;
	float FirstRowTop = 235.0f;
	float RowStep = 100.0f;
	float NameX = 110.0f;
	float NameOffsetY = 12.0f;
	float InfoOffsetY = -18.0f;
	float PipsX = 520.0f;
	float PipSize = 18.0f;
	float PipStep = 26.0f;
	float PipBorder = 2.0f;
	float BuyX = 850.0f;
	float BuyWidth = 220.0f;
	float BuyHeight = 56.0f;
	float BackBottom = 56.0f;
	float ArrowSize = 14.0f;
	float ArrowGap = 6.0f;
	model::Color ArrowColor{ 255, 204, 64 };
	model::Color PipEdge{ 89, 102, 128 };
	model::Color PipFull{ 255, 204, 64 };
	model::Color PipEmpty{ 26, 28, 38 };
};

/** The loading screen. */
struct LoadingLayout
{
	float TitleFontSize = 84.0f;
	float StatusFontSize = 24.0f;
	float TitleOffsetY = 40.0f;
	float StatusOffsetY = -50.0f;
	float BarOffsetY = -100.0f;
	float BarWidth = 480.0f;
	float BarHeight = 16.0f;
	float BarBorder = 2.0f;
	float MinShowTime = 0.6f;
	float StartDelay = 0.05f;
};

/**
 * Layout of the menu screens (configs/ui/menu.json): main menu, settings, upgrades, loading screen.
 * The initial values are the fallbacks for missing keys.
 */
struct MenuLayout
{
	MainMenuLayout Main;
	SettingsLayout Settings;
	UpgradesLayout Upgrades;
	LoadingLayout Loading;

	// OutMissing gets the path of every missing key (its default is kept).
	static MenuLayout FromConfig(const model::IConfigNode* InNode, std::vector<std::string>& OutMissing);
};

} // namespace tanks::viewmodel
