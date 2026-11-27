#pragma once

#include <string>

namespace game
{
namespace tests
{
/**
 * Times the wall merge (engine-free baker and field) at several pixel sizes per cell: the whole chunk bake, a
 * region bake as after one destroyed cell, the shadow layer and the field snapshot copy. Returns a text table
 * for the test report and the log; the slowest part of each row is marked.
 */
std::string RunTileMergeBenchmark();
} // namespace tests
} // namespace game
