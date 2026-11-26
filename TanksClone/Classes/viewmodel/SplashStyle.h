#pragma once

#include <string>

#include "model/ConfigNode.h"
#include "model/MathTypes.h"
#include "templates/TContainers.h"

namespace game::viewmodel
{
/**
 * Look and timing of the startup splash (configs/ui/splash.json). The splash paints before the strings load, so its
 * two lines of text live here too. The initial values are the fallbacks for missing keys.
 */
struct SplashStyle
{
	int Width = 480;
	float MinShowTime = 1.5f;
	std::string Title = "TANKS CLONE";
	std::string Status = "Loading...";
	std::string FontFace = "Segoe UI";
	model::Color Background{ 14, 17, 24 };
	model::Color TitleColor{ 255, 205, 70 };
	model::Color StatusColor{ 150, 150, 160 };
	model::Color Border{ 70, 80, 100 };
	int BorderWidth = 2;
	int TitleHeightDivisor = 7;
	int StatusHeightDivisor = 18;
	float TitleAreaEnd = 0.6f;
	float StatusAreaEnd = 0.8f;

	static SplashStyle FromConfig(const model::IConfigNode* InNode, TArray<std::string>& OutMissing);
};
} // namespace game::viewmodel
