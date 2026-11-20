#pragma once

#include <string>
#include <string_view>

namespace tanks
{
namespace tests
{

// "TanksClone.exe --showcase <folder>" (Debug and Test builds): walks through the screens of the game and saves
// screenshots of them into the folder (menu, settings, upgrades, editor, the bot playing every level, the boss),
// then a 5-second clip of the bot playing as numbered frames in <folder>/clip/ (30 per second of game time,
// whatever the capture speed), and quits. tools/make_clip.ps1 turns the frames into a video.
// Uses a temporary profile; the player's own files are not touched.
bool IsShowcaseMode(std::string& InFolder);
void RunShowcase(std::string_view InFolder);

} // namespace tests
} // namespace tanks
