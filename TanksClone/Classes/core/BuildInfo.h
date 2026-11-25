#pragma once

namespace game
{
/**
 * Which build is running. Debug tools (the --test mode, engine stats on F3) exist in the Debug build
 * and in the Test build (Release optimizations, TANKS_DEBUG_TOOLS defined); the Release build has none.
 */
#if !defined(NDEBUG) || defined(TANKS_DEBUG_TOOLS)
constexpr bool bHasDebugTools = true;
#else
constexpr bool bHasDebugTools = false;
#endif
} // namespace game
