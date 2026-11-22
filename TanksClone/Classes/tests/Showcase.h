#pragma once

#include <optional>
#include <string>
#include <string_view>

#include "services/DisplaySettings.h"

namespace tanks
{
namespace tests
{

// "TanksClone.exe --showcase <folder>" (Debug and Test builds): walks through the screens of the game and saves
// screenshots of them into the folder (menu, settings, upgrades, editor, the bot playing every level, the boss),
// then a 5-second clip of the bot playing as numbered frames in <folder>/clip/ (30 per second of game time,
// whatever the capture speed), and quits. tools/make_clip.ps1 turns the frames into a video.
// Uses a temporary profile; the player's own files are not touched.
// "--showcase <folder> --look" takes only the tank look gallery picture; "--look-gallery" opens that gallery and stays.
// "--window=<width>x<height>" sets the window's inner size first, e.g. to take the pictures at 4:3 or 16:9.
bool IsShowcaseMode(std::string& InFolder);
std::optional<PixelSize> ParseShowcaseWindow(std::string_view InCommandLine);
void RunShowcase(std::string_view InFolder);

} // namespace tests
} // namespace tanks
