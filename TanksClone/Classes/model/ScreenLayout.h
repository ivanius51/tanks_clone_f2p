#pragma once

#include <string_view>

#include "model/MathTypes.h"

namespace tanks::model
{
/** A point of the screen that floating UI sticks to. */
enum class ScreenAnchor
{
	BottomLeft,
	Bottom,
	BottomRight,
	Left,
	Center,
	Right,
	TopLeft,
	Top,
	TopRight
};

bool EnumFromName(std::string_view InName, ScreenAnchor& OutAnchor);

/**
 * Fits the design area (the size the screens are laid out for) into a window of any aspect ratio:
 * the design area is scaled uniformly until it touches two edges and is centred; the visible area grows
 * along the other axis instead of showing black bars. Everything is in design units, with the design
 * area at 0,0; the visible area may start below zero.
 */
class ScreenLayout
{
public:
	ScreenLayout(const Vec2f& InDesignSize, const Vec2f& InFrameSize);

	float GetScale() const;
	const Vec2f& GetDesignSize() const;
	Vec2f GetVisibleSize() const;
	Vec2f GetDesignOffset() const;
	RectF GetVisibleRect() const;
	Vec2f GetAnchor(ScreenAnchor InAnchor) const;
	Vec2f Place(ScreenAnchor InAnchor, const Vec2f& InOffset) const;

private:
	Vec2f DesignSize;
	Vec2f FrameSize;
	float Scale = 1.0f;
};
} // namespace tanks::model
