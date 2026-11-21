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

// Config names: "bottom_left", "top", "center"... False for an unknown name.
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

	// Window pixels per design unit.
	float GetScale() const;
	const Vec2f& GetDesignSize() const;
	// The whole window in design units: never smaller than the design size, the aspect of the window.
	Vec2f GetVisibleSize() const;
	// Where the design area's corner is inside the visible area.
	Vec2f GetDesignOffset() const;
	// The visible area in design coordinates.
	RectF GetVisibleRect() const;
	// The anchor's point of the visible area, in design coordinates.
	Vec2f GetAnchor(ScreenAnchor InAnchor) const;
	// A point InOffset away from the anchor (positive X right, positive Y up).
	Vec2f Place(ScreenAnchor InAnchor, const Vec2f& InOffset) const;

private:
	Vec2f DesignSize;
	Vec2f FrameSize;
	float Scale = 1.0f;
};

} // namespace tanks::model
