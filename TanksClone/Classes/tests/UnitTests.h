#pragma once

#include "tests/TestFramework.h"

namespace game
{
namespace tests
{

// Tests of single parts of the game: stats, buffs, enemy and level data, tanks, upgrades.
// They need the game data to be loaded (loadGameData) and run inside a running scene.
TestSuite CreateUnitTests();

} // namespace tests
} // namespace game
