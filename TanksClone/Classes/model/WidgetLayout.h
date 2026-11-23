#pragma once

#include <string_view>
#include <vector>

#include "model/MathTypes.h"

namespace tanks::model
{

enum class LayoutKind
{
	Stack,  // every child centred on the container, shifted by its offset
	Row,    // children left to right with a gap, as one group
	Column, // children top to bottom with a gap, as one group
	Anchor  // every child pinned to a point of the container (0..1 per axis) plus an offset
};

enum class LayoutAlign
{
	Start,  // left / top
	Center,
	End     // right / bottom
};

bool EnumFromName(std::string_view InName, LayoutKind& OutKind);
bool EnumFromName(std::string_view InName, LayoutAlign& OutAlign);

/** How one container places its children. A zero FixedSize axis means "as big as the content". */
struct LayoutRules
{
	LayoutKind Kind = LayoutKind::Stack;
	float Gap = 0.0f;
	Vec2f Padding;
	Vec2f FixedSize;
	Vec2f MinSize;
	LayoutAlign MainAlign = LayoutAlign::Center;
	LayoutAlign CrossAlign = LayoutAlign::Center;
};

/** One child to place: its measured Size and placement hints in; Position (its centre from the container centre, Y up) out. */
struct LayoutChild
{
	Vec2f Size;
	Vec2f Anchor{ 0.5f, 0.5f };
	Vec2f Offset;
	// Takes the inner size of the container (a background); does not count when measuring it.
	bool bFill = false;
	Vec2f Position;
};

/** The pure layout algorithm of the widget factory: no engine types, so it is unit-tested directly. */
namespace widget_layout
{

// Sizes the container and places every child. Returns the container size.
Vec2f Arrange(const LayoutRules& InRules, std::vector<LayoutChild>& InOutChildren);

// The factor (at most 1, at least InMinFactor) that shrinks a block of InScalable height so that it and a fixed
// InFixed height fit InAvailable together.
float FitFactor(float InAvailable, float InFixed, float InScalable, float InMinFactor);

} // namespace widget_layout

} // namespace tanks::model
