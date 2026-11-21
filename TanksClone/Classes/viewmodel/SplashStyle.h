#pragma once

#include <string>
#include <vector>

#include "model/ConfigNode.h"
#include "model/MathTypes.h"

namespace tanks::viewmodel
{

/**
 * Look and timing of the startup splash (configs/ui/splash.json). The splash paints before the strings load, so its
 * two lines of text live here too. The initial values are the fallbacks for missing keys.
 */
struct SplashStyle
{
	int Width = 480;
	// The game window stays hidden at least this long, so a quick start doesn't flash the splash.
	float MinShowTime = 1.5f;
	std::string Title = "TANKS CLONE";
	std::string Status = "Loading...";
	std::string FontFace = "Segoe UI";
	model::Color Background{ 14, 17, 24 };
	model::Color TitleColor{ 255, 205, 70 };
	model::Color StatusColor{ 150, 150, 160 };
	model::Color Border{ 70, 80, 100 };
	int BorderWidth = 2;
	// Font heights as parts of the window height.
	int TitleHeightDivisor = 7;
	int StatusHeightDivisor = 18;
	// Where the title area ends and the status area ends, as parts of the window height from the top.
	float TitleAreaEnd = 0.6f;
	float StatusAreaEnd = 0.8f;

	// OutMissing gets the path of every missing key (its default is kept).
	static SplashStyle FromConfig(const model::IConfigNode* InNode, std::vector<std::string>& OutMissing);
};

} // namespace tanks::viewmodel
