#pragma once

#include <string>
#include <vector>

#include "model/ConfigNode.h"
#include "model/MathTypes.h"
#include "model/UpgradeTree.h"

namespace tanks::viewmodel
{

/** The main menu screen: the title, the info line and the buttons are one centred group (configs/ui/widgets/main_menu.json). */
struct MainMenuLayout
{
	float ScreenMargin = 16.0f;
	float MinFitFactor = 0.5f;
	float ButtonWidth = 380.0f;
	float ButtonHeight = 52.0f;
	float ButtonSpacing = 12.0f;
	float ListOffsetY = -60.0f;
};

/** The settings list, as a screen or over a paused level. */
struct SettingsLayout
{
	float ButtonWidth = 560.0f;
	float ButtonHeight = 48.0f;
	float ButtonSpacing = 8.0f;
	float ListCenterOffset = 40.0f;
	float ScreenMargin = 16.0f;
	float MinFitFactor = 0.5f;
	int VolumeStep = 5;
	int DeviceNameChars = 32;
};

/** A drawing of the tank on the upgrades screen, scaled to Width. */
struct UpgradeViewLayout
{
	std::string Frame;
	model::Vec2f Center;
	float Width = 0.0f;
};

/** A weapon tab above the turret: picking it shows that weapon's branch in the box. */
struct UpgradeWeaponTab
{
	std::string Branch;
	std::string NameKey;
	std::string Icon;
};

/** The upgrades shop, a blueprint of the tank with round upgrade nodes. Y positions of the panel count down from the top. */
struct UpgradesLayout
{
	float PanelX = 760.0f;
	float PanelWidth = 248.0f;
	float ButtonWidth = 220.0f;
	float ButtonHeight = 56.0f;
	float NodeSize = 60.0f;
	float NameGap = 14.0f;
	float RingWidth = 5.0f;
	float RingGap = 3.0f;
	float RingSegmentGap = 0.25f;
	float LinkWidth = 2.0f;
	float FocusWidth = 3.0f;
	float FocusGap = 10.0f;
	float GridStep = 24.0f;
	int GridMajorEvery = 5;
	model::Color BackgroundColor{ 12, 35, 64 };
	model::Color GridColor{ 22, 58, 98 };
	model::Color GridMajorColor{ 32, 82, 134 };
	model::Color PanelColor{ 10, 29, 54 };
	model::Color EdgeColor{ 95, 180, 238 };
	model::Color BoxColor{ 13, 42, 76 };
	model::Color NodeFill{ 12, 35, 64 };
	model::Color NodeReady{ 95, 180, 238 };
	model::Color NodeDone{ 255, 204, 64 };
	model::Color NodeLocked{ 50, 80, 115 };
	model::Color LinkOn{ 255, 204, 64 };
	model::Color LinkOff{ 95, 180, 238 };
	model::Color RingEmpty{ 42, 68, 102 };
	model::Color RingFull{ 255, 204, 64 };
	model::Color FocusColor{ 255, 255, 255 };
	model::Vec2f TurretPoint{ 412.0f, 572.0f };
	model::Vec2f BoxOrigin{ 606.0f, 440.0f };
	model::Vec2f BoxSize{ 148.0f, 404.0f };
	float WeaponsOpenSeconds = 0.3f;
	float WeaponsStaggerSeconds = 0.03f;
	std::vector<UpgradeViewLayout> Views;
	std::vector<model::UpgradeBranch> Branches;
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
