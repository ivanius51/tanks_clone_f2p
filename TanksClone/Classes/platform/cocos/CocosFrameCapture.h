#pragma once

#include <cstdint>

#include "templates/TContainers.h"

namespace cocos2d { class Node; }

namespace game::view::cocos
{
/** Draws a node into an offscreen texture and hands back its pixels, so the picture never depends on the window (hidden, covered, resized). */
class CocosFrameCapture
{
public:
	/** Fills OutRgba with width * height * 4 bytes, top row first; false when the node has no size or the read failed. */
	static bool Capture(cocos2d::Node* InRoot, TArray<std::uint8_t>& OutRgba, int& OutWidth, int& OutHeight);
};
} // namespace game::view::cocos
